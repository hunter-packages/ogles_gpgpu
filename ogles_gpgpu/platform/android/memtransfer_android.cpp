#include "memtransfer_android.h"
#include "../../common/tools.h"

#include <android/native_window.h>
#include <sys/system_properties.h>

#include <dlfcn.h>
#include <stdlib.h>

// Check Android API at run-time: https://stackoverflow.com/a/33268925
// Android Nougat API=24 blocks dlopen.
static int get_android_api() {
    static int api(-1);
    if (api < 0) {
#if __ANDROID_API__ < 21
        char value[PROP_VALUE_MAX] = { '\0' };
        int length = __system_property_get("ro.build.version.sdk", value);
        api = atoi(value);
#else
        api = popen("getprop ro.build.version.sdk");
#endif
    }

    return api;
}

extern "C" void* fake_dlopen(const char* filename, int flags);
extern "C" int fake_dlclose(void* handle);
extern "C" void* fake_dlsym(void* handle, const char* symbol);

void* try_dlopen(const char* lib, int flags) {
    if (get_android_api() < 24) {
        void* dlEGLhndl = fake_dlopen(lib, flags);
    } else {
#ifdef __arm__
        std::string path = "/system/lib/";
#else
        std::string path = "/system/lib64/";
#endif
        path += lib;
        void* dlEGLhndl = fake_dlopen(path.c_str(), flags);
    }
}

void* try_dlsym(void* handle, const char* symbol) {
    if (get_android_api() < 24) {
        return dlsym(handle, symbol);
    } else {
        return fake_dlsym(handle, symbol);
    }
}

int try_dlclose(void* handle) {
    if (get_android_api() < 24) {
        return dlclose(handle);
    } else {
        return fake_dlclose(handle);
    }
}

typedef struct android_native_base_t {
    /* a magic value defined by the actual EGL native type */
    int magic;

    /* the sizeof() of the actual EGL native type */
    int version;

    void* reserved[4];

    /* reference-counting interface */
    void (*incRef)(struct android_native_base_t* base);
    void (*decRef)(struct android_native_base_t* base);
} android_native_base_t;

typedef struct native_handle {
    int version; /* sizeof(native_handle_t) */
    int numFds; /* number of file-descriptors at &data[0] */
    int numInts; /* number of ints at &data[numFds] */
    int data[0]; /* numFds + numInts ints */
} native_handle_t;

typedef const native_handle_t* buffer_handle_t;

typedef struct ANativeWindowBuffer {
    struct android_native_base_t common;

    int width;
    int height;
    int stride;
    int format;
    int usage;

    void* reserved[2];

    buffer_handle_t handle;

    void* reserved_proc[8];

} ANativeWindowBuffer_t;

using namespace ogles_gpgpu;

// necessary definitions for Android GraphicBuffer

enum {
    /* buffer is never read in software */
    GRALLOC_USAGE_SW_READ_NEVER = 0x00000000,
    /* buffer is rarely read in software */
    GRALLOC_USAGE_SW_READ_RARELY = 0x00000002,
    /* buffer is often read in software */
    GRALLOC_USAGE_SW_READ_OFTEN = 0x00000003,
    /* mask for the software read values */
    GRALLOC_USAGE_SW_READ_MASK = 0x0000000F,

    /* buffer is never written in software */
    GRALLOC_USAGE_SW_WRITE_NEVER = 0x00000000,
    /* buffer is never written in software */
    GRALLOC_USAGE_SW_WRITE_RARELY = 0x00000020,
    /* buffer is never written in software */
    GRALLOC_USAGE_SW_WRITE_OFTEN = 0x00000030,
    /* mask for the software write values */
    GRALLOC_USAGE_SW_WRITE_MASK = 0x000000F0,

    /* buffer will be used as an OpenGL ES texture */
    GRALLOC_USAGE_HW_TEXTURE = 0x00000100,
    /* buffer will be used as an OpenGL ES render target */
    GRALLOC_USAGE_HW_RENDER = 0x00000200,
    /* buffer will be used by the 2D hardware blitter */
    GRALLOC_USAGE_HW_2D = 0x00000400,
    /* buffer will be used with the framebuffer device */
    GRALLOC_USAGE_HW_FB = 0x00001000,
    /* mask for the software usage bit-mask */
    GRALLOC_USAGE_HW_MASK = 0x00001F00,
};

enum {
    HAL_PIXEL_FORMAT_RGBA_8888 = 1,
    HAL_PIXEL_FORMAT_RGBX_8888 = 2,
    HAL_PIXEL_FORMAT_RGB_888 = 3,
    HAL_PIXEL_FORMAT_RGB_565 = 4,
    HAL_PIXEL_FORMAT_BGRA_8888 = 5,
    HAL_PIXEL_FORMAT_RGBA_5551 = 6,
    HAL_PIXEL_FORMAT_RGBA_4444 = 7,
};

#define OG_DL_FUNC(hndl, fn, type) (type) try_dlsym(hndl, fn)
#define OG_DL_FUNC_CHECK(hndl, fn_ptr, fn)                                                          \
    if (!fn_ptr) {                                                                                  \
        OG_LOGERR("MemTransferAndroid", "could not dynamically link func '%s': %s", fn, dlerror()); \
        try_dlclose(hndl);                                                                          \
        return false;                                                                               \
    }

#pragma mark static initializations and methods

GraphicBufferFnCtor MemTransferAndroid::graBufCreate = NULL;
GraphicBufferFnDtor MemTransferAndroid::graBufDestroy = NULL;
GraphicBufferFnGetNativeBuffer MemTransferAndroid::graBufGetNativeBuffer = NULL;
GraphicBufferFnLock MemTransferAndroid::graBufLock = NULL;
GraphicBufferFnUnlock MemTransferAndroid::graBufUnlock = NULL;

EGLExtFnCreateImage MemTransferAndroid::imageKHRCreate = NULL;
EGLExtFnDestroyImage MemTransferAndroid::imageKHRDestroy = NULL;

EGLExtFnCreateSyncKHR MemTransferAndroid::createKHRSync = NULL;
EGLExtFnDestroySyncKHR MemTransferAndroid::destroyKHRSync = NULL;
EGLExtFnClientWaitSyncKHR MemTransferAndroid::waitKHRSync = NULL;

#if OPENGLES_GPGPU_HAS_NATIVE_FENCE_FD_ANDROID
EGLExtFnDupNativeFenceFDANDROID MemTransferAndroid::dupNativeFenceFDANDROID = NULL;
#endif

bool MemTransferAndroid::initPlatformOptimizations() {
    // load necessary EGL extension functions
    void* dlEGLhndl = try_dlopen("libEGL.so", RTLD_LAZY);
    if (!dlEGLhndl) {
        OG_LOGERR("MemTransferAndroid", "could not load EGL library: %s", dlerror());
        return false;
    }

    imageKHRCreate = OG_DL_FUNC(dlEGLhndl, "eglCreateImageKHR", EGLExtFnCreateImage);
    OG_DL_FUNC_CHECK(dlEGLhndl, imageKHRCreate, "eglCreateImageKHR");

    imageKHRDestroy = OG_DL_FUNC(dlEGLhndl, "eglDestroyImageKHR", EGLExtFnDestroyImage);
    OG_DL_FUNC_CHECK(dlEGLhndl, imageKHRDestroy, "eglDestroyImageKHR");

#if OPENGLES_GPGPU_HAS_NATIVE_FENCE_FD_ANDROID
    // Load sync points for ANDROID:
    dupNativeFenceFDANDROID = OG_DL_FUNC(dlEGLhndl, "eglDupNativeFenceFDANDROID", EGLExtFnDupNativeFenceFDANDROID);
//OG_DL_FUNC_CHECK(dlEGLhndl, dupNativeFenceFDANDROID, "eglDupNativeFenceFDANDROID");
#endif

    // Try loading egl{Create,Destroy,ClientWait}SyncKHR, else we will resort to glFinish():
    createKHRSync = OG_DL_FUNC(dlEGLhndl, "eglCreateSyncKHR", EGLExtFnCreateSyncKHR);
    //OG_DL_FUNC_CHECK(dlEGLhndl, createKHRSync, "eglCreateSyncKHR");
    if (createKHRSync) {
        destroyKHRSync = OG_DL_FUNC(dlEGLhndl, "eglDestroySyncKHR", EGLExtFnDestroySyncKHR);
        //OG_DL_FUNC_CHECK(dlEGLhndl, destroyKHRSync, "eglDestroySyncKHR");
        if (destroyKHRSync) {
            waitKHRSync = OG_DL_FUNC(dlEGLhndl, "eglClientWaitSyncKHR", EGLExtFnClientWaitSyncKHR);
            //OG_DL_FUNC_CHECK(dlEGLhndl, destroyKHRSync, "eglClientWaitSyncKHR");
        }
    }

    try_dlclose(dlEGLhndl);

    // load necessary Android GraphicBuffer functions
    void* dlUIhndl = try_dlopen("libui.so", RTLD_LAZY);
    if (!dlUIhndl) {
        OG_LOGERR("MemTransferAndroid", "could not load Android UI library: %s", dlerror());
        return false;
    }

    graBufCreate = OG_DL_FUNC(dlUIhndl, "_ZN7android13GraphicBufferC1Ejjij", GraphicBufferFnCtor);
    OG_DL_FUNC_CHECK(dlUIhndl, graBufCreate, "_ZN7android13GraphicBufferC1Ejjij");

    graBufDestroy = OG_DL_FUNC(dlUIhndl, "_ZN7android13GraphicBufferD1Ev", GraphicBufferFnDtor);
    OG_DL_FUNC_CHECK(dlUIhndl, graBufDestroy, "_ZN7android13GraphicBufferD1Ev");

    graBufGetNativeBuffer = OG_DL_FUNC(dlUIhndl, "_ZNK7android13GraphicBuffer15getNativeBufferEv", GraphicBufferFnGetNativeBuffer);
    OG_DL_FUNC_CHECK(dlUIhndl, graBufGetNativeBuffer, "_ZNK7android13GraphicBuffer15getNativeBufferEv");

    graBufLock = OG_DL_FUNC(dlUIhndl, "_ZN7android13GraphicBuffer4lockEjPPv", GraphicBufferFnLock);
    OG_DL_FUNC_CHECK(dlUIhndl, graBufLock, "_ZN7android13GraphicBuffer4lockEjPPv");

    graBufUnlock = OG_DL_FUNC(dlUIhndl, "_ZN7android13GraphicBuffer6unlockEv", GraphicBufferFnUnlock);
    OG_DL_FUNC_CHECK(dlUIhndl, graBufUnlock, "_ZN7android13GraphicBuffer6unlockEv");

    try_dlclose(dlUIhndl);

    // all done
    OG_LOGINF("MemTransferAndroid", "static init completed");

    return true;
}

#pragma mark constructor / deconstructor

MemTransferAndroid::~MemTransferAndroid() {
    // release in- and outputs
    releaseInput();
    releaseOutput();
}

#pragma mark public methods

void MemTransferAndroid::releaseInput() {
    // release input image
    if (inputImage) {
        OG_LOGINF("MemTransferAndroid", "releasing input image");
        imageKHRDestroy(EGL_DEFAULT_DISPLAY, inputImage);
        free(inputImage);
        inputImage = NULL;
    }

    // release android graphic buffer handle for input
    if (inputGraBufHndl) {
        OG_LOGINF("MemTransferAndroid", "releasing graphic buffer handle for input");
        graBufDestroy(inputGraBufHndl);
        free(inputGraBufHndl);

        inputGraBufHndl = NULL;
        inputNativeBuf = NULL; // reset weak-ref pointer to NULL
    }
}

void MemTransferAndroid::releaseOutput() {
    // release output image
    if (outputImage) {
        OG_LOGINF("MemTransferAndroid", "releasing output image");
        imageKHRDestroy(EGL_DEFAULT_DISPLAY, outputImage);
        free(outputImage);
        outputImage = NULL;
    }

    // release android graphic buffer handle for output
    if (outputGraBufHndl) {
        OG_LOGINF("MemTransferAndroid", "releasing graphic buffer handle for output");
        graBufDestroy(outputGraBufHndl);
        free(outputGraBufHndl);

        outputGraBufHndl = NULL;
        outputNativeBuf = NULL; // reset weak-ref pointer to NULL
    }
}

void MemTransferAndroid::init() {
    assert(!initialized);

    // call parent init
    MemTransfer::init();

    mEGLDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
}

GLuint MemTransferAndroid::prepareInput(int inTexW, int inTexH, GLenum inputPxFormat, void* inputDataPtr) {
    assert(initialized && inTexW > 0 && inTexH > 0);

    if (inputDataPtr == NULL && inputW == inTexW && inputH == inTexH && inputPixelFormat == inputPxFormat) {
        return inputTexId; // no change
    }

    if (preparedInput) { // already prepared -- release buffers!
        releaseInput();
    }

    // set attributes
    inputW = inTexW;
    inputH = inTexH;
    inputPixelFormat = inputPxFormat;

    // generate texture id for input
    glGenTextures(1, &inputTexId);

    if (inputTexId <= 0) {
        OG_LOGERR("MemTransferAndroid", "error generating input texture id");
        return 0;
    }

    // handle input pixel format
    int nativePxFmt = HAL_PIXEL_FORMAT_RGBA_8888;
    if (inputPixelFormat != GL_RGBA) {
        OG_LOGERR("MemTransferAndroid", "warning: only GL_RGBA is valid as input pixel format");
    }

    // create graphic buffer
    inputGraBufHndl = malloc(OG_ANDROID_GRAPHIC_BUFFER_SIZE);
    graBufCreate(inputGraBufHndl, inputW, inputH, nativePxFmt,
        GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_SW_WRITE_OFTEN); // is used as OpenGL texture and will be written often

    // get window buffer
    inputNativeBuf = (struct ANativeWindowBuffer*)graBufGetNativeBuffer(inputGraBufHndl);

    if (!inputNativeBuf) {
        OG_LOGERR("MemTransferAndroid", "error getting native window buffer for input");
        return 0;
    }

    // create image for reading back the results
    EGLint eglImgAttrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE, EGL_NONE };
    inputImage = imageKHRCreate(mEGLDisplay,
        EGL_NO_CONTEXT,
        EGL_NATIVE_BUFFER_ANDROID,
        (EGLClientBuffer)inputNativeBuf,
        eglImgAttrs); // or NULL as last param?

    if (!inputImage) {
        OG_LOGERR("MemTransferAndroid", "error creating image KHR for input");
        return 0;
    }

    preparedInput = true;

    OG_LOGINF("MemTransferAndroid", "successfully prepared input with texture id %d", inputTexId);

    return inputTexId;
}

GLuint MemTransferAndroid::prepareOutput(int outTexW, int outTexH) {
    assert(initialized && outTexW > 0 && outTexH > 0);

    if (outputW == outTexW && outputH == outTexH) {
        return outputTexId; // no change
    }

    if (preparedOutput) { // already prepared -- release buffers!
        releaseOutput();
    }

    // set attributes
    outputW = outTexW;
    outputH = outTexH;

    // generate texture id for input
    glGenTextures(1, &outputTexId);

    if (outputTexId <= 0) {
        OG_LOGERR("MemTransferAndroid", "error generating output texture id");
        return 0;
    }

    // will bind the texture, too:
    setCommonTextureParams(outputTexId);

    Tools::checkGLErr("MemTransferAndroid", "fbo texture parameters");

    // create empty texture space on GPU
    glTexImage2D(GL_TEXTURE_2D, 0,
        GL_RGBA,
        outputW, outputH, 0,
        inputPixelFormat, GL_UNSIGNED_BYTE,
        NULL); // we do not need to pass texture data -> it will be generated!

    Tools::checkGLErr("MemTransferAndroid", "fbo texture creation");

    // create graphic buffer
    outputGraBufHndl = malloc(OG_ANDROID_GRAPHIC_BUFFER_SIZE);
    graBufCreate(outputGraBufHndl, outputW, outputH, HAL_PIXEL_FORMAT_RGBA_8888,
        GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_SW_READ_OFTEN); // is render target and will be read often

    // get window buffer
    outputNativeBuf = (struct ANativeWindowBuffer*)graBufGetNativeBuffer(outputGraBufHndl);

    if (!outputNativeBuf) {
        OG_LOGERR("MemTransferAndroid", "error getting native window buffer for output");
        return 0;
    }

    // create image for reading back the results
    EGLint eglImgAttrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE, EGL_NONE };
    outputImage = imageKHRCreate(mEGLDisplay,
        EGL_NO_CONTEXT,
        EGL_NATIVE_BUFFER_ANDROID,
        (EGLClientBuffer)outputNativeBuf,
        eglImgAttrs); // or NULL as last param?

    if (!outputImage) {
        OG_LOGERR("MemTransferAndroid", "error creating image KHR for output");
        return 0;
    }

    preparedOutput = true;

    OG_LOGINF("MemTransferAndroid", "successfully prepared output with texture id %d", outputTexId);

    return outputTexId;
}

void MemTransferAndroid::flush(uint32_t us) {
    // https://github.com/android/platform_frameworks_base/blob/master/services/core/jni/com_android_server_AssetAtlasService.cpp#L169-L185
    EGLSyncKHR sync;

    if (createKHRSync && destroyKHRSync) {

#if OPENGLES_GPGPU_HAS_NATIVE_FENCE_FD_ANDROID
        // Give priority to eglDupNativeFenceFDANDROID
        if (dupNativeFenceFDANDROID) {
            sync = eglCreateSyncKHR(mEGLDisplay, EGL_SYNC_NATIVE_FENCE_ANDROID, NULL);
            glFlush(); // getRenderEngine().flush();

            if (sync != EGL_NO_SYNC_KHR) {
                auto syncFd = eglDupNativeFenceFDANDROID(mEGLDisplay, sync);
                if (syncFd == EGL_NO_NATIVE_FENCE_FD_ANDROID) {
                    OG_LOGERR("MemTransferAndroid::flush:", "failed to dup sync khr object");
                    syncFd = -1;
                }
                eglDestroySyncKHR(mEGLDisplay, sync);
                return;
            }
        }
#endif

        // Fallback mechanism:
        if (waitKHRSync) {
            sync = createKHRSync(mEGLDisplay, EGL_SYNC_FENCE_KHR, NULL);
            if (sync != EGL_NO_SYNC_KHR) {
                EGLint result = waitKHRSync(mEGLDisplay, sync, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, us);
                EGLint eglErr = eglGetError();
                if (result == EGL_TIMEOUT_EXPIRED_KHR) {
                    OG_LOGERR("MemTransferAndroid::flush:", "fence wait timed out");
                } else if (eglErr != EGL_SUCCESS) {
                    OG_LOGERR("MemTransferAndroid::flush:", "error waiting on EGL fence: %#x", eglErr);
                }
                destroyKHRSync(mEGLDisplay, sync);
                return;
            } else {
                OG_LOGERR("MemTransferAndroid::flush:", "captureScreen: error creating EGL fence: %#x", eglGetError());
            }
        }
    }

    //  Use glFinish() as a last resort
    glFinish();
}

void MemTransferAndroid::toGPU(const unsigned char* buf) {
    assert(preparedInput && inputImage && inputTexId > 0 && buf);

    // bind the input texture
    glBindTexture(GL_TEXTURE_2D, inputTexId);

    // activate the image KHR for the input
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, inputImage);

    Tools::checkGLErr("MemTransferAndroid", "call to glEGLImageTargetTexture2DOES() for input");

    // lock the graphics buffer at graphicsPtr
    unsigned char* graphicsPtr = (unsigned char*)lockBufferAndGetPtr(BUF_TYPE_INPUT);

    // copy whole image from "buf" to "graphicsPtr"
    memcpy(graphicsPtr, buf, inputW * inputH * 4);

    // unlock the graphics buffer again
    unlockBuffer(BUF_TYPE_INPUT);
}

void MemTransferAndroid::fromGPU(unsigned char* buf) {
    assert(preparedOutput && outputImage && outputTexId > 0 && buf);

    // bind the output texture
    glBindTexture(GL_TEXTURE_2D, outputTexId);

    // activate the image KHR for the output
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, outputImage);

    Tools::checkGLErr("MemTransferAndroid", "call to glEGLImageTargetTexture2DOES() for output");

    // lock the graphics buffer at graphicsPtr
    const unsigned char* graphicsPtr = (const unsigned char*)lockBufferAndGetPtr(BUF_TYPE_OUTPUT);

    // copy whole image from "graphicsPtr" to "buf"
    memcpy(buf, graphicsPtr, outputW * outputH * 4);

    // unlock the graphics buffer again
    unlockBuffer(BUF_TYPE_OUTPUT);
}

// TODO: Move this to mem_transfer_optimized
void MemTransferAndroid::fromGPU(FrameDelegate& delegate) {
    // bind the texture
    glBindTexture(GL_TEXTURE_2D, outputTexId);

    // activate the image KHR for the output
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, outputImage);

    Tools::checkGLErr("MemTransferAndroid", "call to glEGLImageTargetTexture2DOES() for output");

    const void* pixelBufferAddr = lockBufferAndGetPtr(BUF_TYPE_OUTPUT);
    delegate({ outputW, outputH }, pixelBufferAddr, bytesPerRow());

    unlockBuffer(BUF_TYPE_OUTPUT);
}

size_t MemTransferAndroid::bytesPerRow() {
    std::stringstream ss;
    return (outputNativeBuf->stride * 4);
}

void* MemTransferAndroid::lockBufferAndGetPtr(BufType bufType) {
    void* hndl;
    int usage;
    unsigned char* memPtr;

    if (bufType == BUF_TYPE_INPUT) {
        hndl = inputGraBufHndl;
        usage = GRALLOC_USAGE_SW_WRITE_OFTEN;
    } else {
        hndl = outputGraBufHndl;
        usage = GRALLOC_USAGE_SW_READ_OFTEN;
    }

    // lock and get pointer
    graBufLock(hndl, usage, &memPtr);

    // check for valid pointer
    if (!memPtr) {
        OG_LOGERR("MemTransferAndroid", "GraphicBuffer lock returned invalid pointer");
    }

    return (void*)memPtr;
}

void MemTransferAndroid::unlockBuffer(BufType bufType) {
    void* hndl = (bufType == BUF_TYPE_INPUT) ? inputGraBufHndl : outputGraBufHndl;

    graBufUnlock(hndl);
}
