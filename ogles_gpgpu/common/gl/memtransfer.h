//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// Author: Markus Konrad <post@mkonrad.net>, Winter 2014/2015
// http://www.mkonrad.net
//
// See LICENSE file in project repository root for the license.
//

/**
 * MemTransfer handles memory transfer and mapping between CPU and GPU memory space.
 */

#ifndef OGLES_GPGPU_COMMON_GL_MEMTRANSFER
#define OGLES_GPGPU_COMMON_GL_MEMTRANSFER

#include "../common_includes.h"

#include <functional>
#include <memory>

#define OGLES_GPGPU_USE_CLASS_READ 1
#define OGLES_GPGPU_USE_CLASS_WRITE 1

namespace ogles_gpgpu {

class IPBO;
class OPBO;
class FBO;

/**
 * MemTransfer handles memory transfer and mapping between CPU and GPU memory space.
 * Input (from CPU to GPU space) and output (from GPU to CPU space) can be set up
 * separately.
 */
class MemTransfer {
public:
    typedef std::function<void(const Size2d& size, const void* pixels, size_t rowStride)> FrameDelegate;

    /**
     * Constructor
     */
    MemTransfer();

    /**
     * Deconstructor
     */
    virtual ~MemTransfer();

    /**
     * Initialize method to be called AFTER the OpenGL context was created.
     */
    virtual void init() {
        initialized = true;
    }

    /**
     * Prepare for input frames of size <inTexW>x<inTexH>. Return a texture id for the input frames.
     */
    virtual GLuint prepareInput(int inTexW, int inTexH, GLenum inputPxFormat = GL_RGBA, void* inputDataPtr = NULL);

    /**
     * Prepare for output frames of size <outTexW>x<outTexH>. Return a texture id for the output frames.
     */
    virtual GLuint prepareOutput(int outTexW, int outTexH);

    /**
     * Set output pixel format.
     */
    virtual void setOutputPixelFormat(GLenum outputPxFormat);

    /**
     * Delete input texture.
     */
    virtual void releaseInput();

    /**
     * Delete output texture.
     */
    virtual void releaseOutput();

    /**
     * Get input texture id.
     */
    virtual GLuint getInputTexId() const {
        return inputTexId;
    }

    /**
     * Get output texture id.
     */
    virtual GLuint getOutputTexId() const {
        return outputTexId;
    }

    /**
     * Get output luminance texture id.
     */
    virtual GLuint getLuminanceTexId() const {
        return luminanceTexId;
    }

    /**
     * Get output chrominance texture id.
     */
    virtual GLuint getChrominanceTexId() const {
        return chrominanceTexId;
    }

    /**
     * Map data in <buf> to GPU.
     */
    virtual void toGPU(const unsigned char* buf);

    /**
     * Map data from GPU to <buf>
     *
     * If OGLES_GPGPU_ES3 is defined, then we have three
     * valid modes of operation:
     *
     * 1) if (<buf> == nullptr), then the transfer for PBO <index>
     * will be initiated asynchronously
     *
     * 2) if (<buf> != nullptr), and this call was preceded
     * by a call for the same index where (<buf> == nullptr)
     * then the call will block on a final memcpy of the 
     * PBO buffer.
     *
     * 3) if (<buf> != nullptr), and this call was not preceded
     * by a call for the same index where (<buf> == nullptr)
     * then the call will perform a syncrhonous memcpy of the
     * PBO buffer
     */
    virtual void fromGPU(unsigned char* buf, int index = 0);

    /**
     * Map data from GPU to <buf>
     *
     * If OGLES_GPGPU_ES3 is defined, then we have three
     * valid modes of operation:
     *
     * 1) if (<delegate> == nullptr), then the transfer for PBO <index>
     * will be initiated asynchronously
     *
     * 2) if (<delegate> != nullptr), and this call was preceded
     * by a call for the same index where (<delegate> == nullptr)
     * then the call will block on a final memcpy of the
     * PBO buffer.
     *
     * 3) if (<delegate> != nullptr), and this call was not preceded
     * by a call for the same index where (<delegate> == nullptr)
     * then the call will perform a syncrhonous memcpy of the
     * PBO buffer
     */
    virtual void fromGPU(const FrameDelegate& delegate, int index = 0);

    /**
     * Get output pixel format (i.e., GL_BGRA or GL_RGBA)
     */
    virtual GLenum getOutputPixelFormat() const {
        return outputPixelFormat;
    }

    /**
     * Inidcates whether or not this MemTransfer implementation
     * support zero copy texture access (i.e., MemTransferIOS)
     */
    virtual bool hasDirectTextureAccess() const {
        return false;
    }

    /**
     * Row stride (in bytes) of the underlying FBO.
     */

    virtual size_t bytesPerRow();

    /**
     * Specify input image format, raw pixels or platform specific image type
     */
    virtual void setUseRawPixels(bool flag) {
        useRawPixels = flag;
    }

    /**
     * Create N framebuffers for asynchronous downloads.
     * (Only supported for >= OpenGL ES 3.0)
     */
    virtual void resizePBO(int count);

    /**
     * Try to initialize platform optimizations. Returns true on success, else false.
     * Is only fully implemented in platform-specialized classes of MemTransfer.
     */
    static bool initPlatformOptimizations();

protected:
    /**
     * bind texture if <texId> > 0 and
     * set clamping (allows NPOT textures)
     */
    virtual void setCommonTextureParams(GLuint texId, GLenum target = GL_TEXTURE_2D);

    bool initialized; // is initialized?

    bool preparedInput; // input is prepared?
    bool preparedOutput; // output is prepared?

    int inputW; // input texture width
    int inputH; // input texture height
    int outputW; // output texture width
    int outputH; // output texture heights

    GLuint inputTexId; // input texture id
    GLuint outputTexId; // output texture id

    GLuint luminanceTexId = 0;
    GLuint chrominanceTexId = 0;

    GLenum inputPixelFormat; // input texture pixel format
    GLenum outputPixelFormat;

    bool useRawPixels = false;

#if defined(OGLES_GPGPU_OPENGL_ES3)
    FBO* fbo = nullptr;

#if OGLES_GPGPU_USE_CLASS_READ
    std::vector<std::unique_ptr<IPBO>> pboReaders;
#else // OGLES_GPGPU_USE_CLASS_READ
    GLuint pboRead;
#endif // OGLES_GPGPU_USE_CLASS_READ

#if OGLES_GPGPU_USE_CLASS_WRITE
    std::unique_ptr<OPBO> pboWrite;
#else // OGLES_GPGPU_USE_CLASS_WRITE
    GLuint pboWrite;
#endif // OGLES_GPGPU_USE_CLASS_WRITE
#endif
};
}

#endif
