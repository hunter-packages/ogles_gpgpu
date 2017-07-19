//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// Author: Markus Konrad <post@mkonrad.net>, Winter 2014/2015
// http://www.mkonrad.net
//
// See LICENSE file in project repository root for the license.
//

#include "memtransfer.h"
#include "fbo.h"

using namespace ogles_gpgpu;

#pragma mark static methods

bool MemTransfer::initPlatformOptimizations() {
    // always return false here. this method is only fully implemented
    // in platform-specialized classes of MemTransfer.
    return false;
}

#pragma mark constructor/deconstructor

#if ANDROID
#  define DFLT_TEXTURE_FORMAT GL_RGBA
#else
#  define DFLT_TEXTURE_FORMAT GL_BGRA
#endif

MemTransfer::MemTransfer() {
    // set defaults
    inputW = inputH = outputW = outputH = 0;
    inputTexId = 0;
    outputTexId = 0;
    initialized = false;
    preparedInput = false;
    preparedOutput = false;
    inputPixelFormat = outputPixelFormat = DFLT_TEXTURE_FORMAT;
}

MemTransfer::~MemTransfer() {
    // release in- and output
    releaseInput();
    releaseOutput();
}

#pragma mark public methods

GLuint MemTransfer::prepareInput(int inTexW, int inTexH, GLenum inputPxFormat, void* inputDataPtr) {
    assert(initialized && inTexW > 0 && inTexH > 0);

    if ((inputDataPtr == nullptr) && (inputW == inTexW) && (inputH == inTexH) && (inputPixelFormat == inputPxFormat)) {
        return inputTexId; // no change
    }

    if (preparedInput) { // already prepared -- release buffers!
        releaseInput();
    }
    
    if (inputPxFormat == 0) {
        return 0;
    }

    // set attributes
    inputW = inTexW;
    inputH = inTexH;
    inputPixelFormat = outputPixelFormat = inputPxFormat;

    // generate texture id
    glGenTextures(1, &inputTexId);

    if (inputTexId == 0) {
        OG_LOGERR("MemTransfer", "no valid input texture generated");
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, inputTexId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#if defined(OGLES_GPGPU_OPENGL_ES3)    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, inputW, inputH, 0, inputPixelFormat, GL_UNSIGNED_BYTE, nullptr);
#endif
    glBindTexture(GL_TEXTURE_2D, 0);
        
#if defined(OGLES_GPGPU_OPENGL_ES3)
    // ::::::: allocate ::::::::::
    size_t pbo_size = inputW * inputH * 4;
    
    glGenBuffers(1, &pboWrite);
    Tools::checkGLErr("MemTransfer", "fromGPU");
    
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboWrite);
    Tools::checkGLErr("MemTransfer", "fromGPU");
    
    glBufferData(GL_PIXEL_UNPACK_BUFFER, pbo_size, 0, GL_STREAM_DRAW);
    Tools::checkGLErr("MemTransfer", "fromGPU");
    
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    Tools::checkGLErr("MemTransfer", "fromGPU");
    
    static const bool forOutput = false;
    fbo = new FBO(forOutput);
#endif
    // done
    preparedInput = true;

    // Texture data to be upladed with Core::setInputData(...)
    return inputTexId;
}

GLuint MemTransfer::prepareOutput(int outTexW, int outTexH) {
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

    // generate texture id
    glGenTextures(1, &outputTexId);

    if (outputTexId == 0) {
        OG_LOGERR("MemTransfer", "no valid output texture generated");
        return 0;
    }

    // will bind the texture, too:
    setCommonTextureParams(outputTexId);

    Tools::checkGLErr("MemTransfer", "fbo texture parameters");

    GLenum rgbFormat = DFLT_TEXTURE_FORMAT;

    // create empty texture space on GPU
    glTexImage2D(GL_TEXTURE_2D, 0,
        GL_RGBA,
        outTexW, outTexH, 0,
        rgbFormat, GL_UNSIGNED_BYTE,
        NULL); // we do not need to pass texture data -> it will be generated!

    Tools::checkGLErr("MemTransfer", "fbo texture creation");

#if defined(OGLES_GPGPU_OPENGL_ES3)
    // ::::::: allocate ::::::::::
    size_t pbo_size = outputW * outputH * 4;
    
    glGenBuffers(1, &pboRead);
    Tools::checkGLErr("MemTransfer", "fromGPU");
    
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pboRead);
    Tools::checkGLErr("MemTransfer", "fromGPU");
    
    glBufferData(GL_PIXEL_PACK_BUFFER, pbo_size, 0, GL_DYNAMIC_READ);
    Tools::checkGLErr("MemTransfer", "fromGPU");
    
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    Tools::checkGLErr("MemTransfer", "fromGPU");
#endif // OGLES_GPGPU_OPENGL_ES3
    
    // done
    preparedOutput = true;

    return outputTexId;
}

void MemTransfer::releaseInput() {
    if (inputTexId > 0) {
        glDeleteTextures(1, &inputTexId);
        inputTexId = 0;
    }

#if defined(OGLES_GPGPU_OPENGL_ES3)
    if (pboWrite > 0) {
        glDeleteBuffers(1, &pboWrite);
        pboWrite = 0;
    }
    
    if(fbo) {
        delete fbo;
        fbo = nullptr;
    }
#endif
}

void MemTransfer::releaseOutput() {
    if (outputTexId > 0) {
        glDeleteTextures(1, &outputTexId);
        outputTexId = 0;
    }
    
#if defined(OGLES_GPGPU_OPENGL_ES3)
    if (pboRead > 0) {
        glDeleteBuffers(1, &pboRead);
        pboRead = 0;
    }
#endif
    
}

void MemTransfer::toGPU(const unsigned char* buf) {
    assert(preparedInput && inputTexId > 0 && buf);

#if defined(OGLES_GPGPU_OPENGL_ES3)
    std::size_t pbo_size = inputW * inputH * 4;

    fbo->bind();
    Tools::checkGLErr("MemTransfer", "toGPU (FBO::bind())");
    
    fbo->attach(inputTexId);
    Tools::checkGLErr("MemTransfer", "toGPU (FBO::attach())");
    
    glBindTexture(GL_TEXTURE_2D, inputTexId); // bind input texture
    Tools::checkGLErr("MemTransfer", "toGPU (glBindTexture)");
    
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboWrite);
    Tools::checkGLErr("MemTransfer", "toGPU (glBindBuffer)");
    
#if defined(OGLES_GPGPU_OSX)
    // TODO: glMapBufferRange does not seem to work in OS X
    GLubyte* ptr = static_cast<GLubyte*>(glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));
    Tools::checkGLErr("MemTransfer", "toGPU (glMapBuffer)");                
#else
    GLubyte* ptr = static_cast<GLubyte*>(glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, pbo_size, GL_MAP_WRITE_BIT));
    Tools::checkGLErr("MemTransfer", "toGPU (glMapBuffer)");                
#endif
    
    if(ptr) {
        memcpy(ptr, buf, pbo_size);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        Tools::checkGLErr("MemTransfer", "toGPU (glMapBuffer)");
    }
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, inputW, inputH, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    Tools::checkGLErr("MemTransfer", "toGPU (glTexSubImage2D)");
    
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    Tools::checkGLErr("MemTransfer", "toGPU (glBindBuffer)");
    
    fbo->unbind();
#else

    // set input texture
    glBindTexture(GL_TEXTURE_2D, inputTexId); // bind input texture
    
    // copy data as texture to GPU (tested: OS X)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, inputW, inputH, 0, inputPixelFormat, GL_UNSIGNED_BYTE, buf);

    // check for error
    Tools::checkGLErr("MemTransfer", "toGPU (glTexImage2D)");
#endif

    setCommonTextureParams(0);
}

void MemTransfer::fromGPU(unsigned char* buf) {
    assert(preparedOutput && outputTexId > 0 && buf);

#if defined(OGLES_GPGPU_OPENGL_ES3)
    size_t pbo_size = outputW * outputH * 4;    

    // TODO: Reuse PBO, put side-by-side with FBO
    // ::::::::: read ::::::::::::
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pboRead);
    Tools::checkGLErr("MemTransfer", "fromGPU");
    
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    Tools::checkGLErr("MemTransfer", "fromGPU");

    // Note glReadPixels last argument == 0 for PBO reads
    glReadPixels(0, 0,outputW, outputH, GL_RGBA, GL_UNSIGNED_BYTE, 0); // GL_BGRA
    Tools::checkGLErr("MemTransfer", "fromGPU");
    
#if defined(OGLES_GPGPU_OSX)
    // TODO: glMapBufferRange does not seem to work in OS X
    GLubyte *ptr = static_cast<GLubyte *>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));
#else
    GLubyte *ptr = static_cast<GLubyte *>(glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, pbo_size, GL_MAP_READ_BIT));
#endif
    Tools::checkGLErr("MemTransfer", "fromGPU");    
    
    memcpy(buf, ptr, pbo_size);

    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    Tools::checkGLErr("MemTransfer", "fromGPU");    
    
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    Tools::checkGLErr("MemTransfer", "fromGPU");

    glDeleteBuffers(1, &pboRead);
    Tools::checkGLErr("MemTransfer", "fromGPU");
#else
    
    glBindTexture(GL_TEXTURE_2D, outputTexId);
    Tools::checkGLErr("MemTransfer", "fromGPU: (glBindTexture)");

    // default (and slow) way using glReadPixels:
    glReadPixels(0, 0, outputW, outputH, outputPixelFormat, GL_UNSIGNED_BYTE, buf);
    Tools::checkGLErr("MemTransfer", "fromGPU: (glReadPixels)");
#endif

}

// The zero copy fromGPU() call is not possibly with generic glReadPixels() access
void MemTransfer::fromGPU(FrameDelegate& delegate) {
    assert(false);
}

size_t MemTransfer::bytesPerRow() {
    return outputW * 4; // assume GL_{BGRA,RGBA}
}

void MemTransfer::setOutputPixelFormat(GLenum outputPxFormat) {
    outputPixelFormat = outputPxFormat;
}

#pragma mark protected methods

void MemTransfer::setCommonTextureParams(GLuint texId, GLenum target) {
    if (texId > 0) {
        Tools::checkGLErr("MemTransfer", "setCommonTextureParams (>glBindTexture)");
        glBindTexture(target, texId);
        Tools::checkGLErr("MemTransfer", "setCommonTextureParams (<glBindTexture)");
    }

    // set clamping (allows NPOT textures)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}
