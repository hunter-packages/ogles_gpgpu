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

// clang-format off
#if defined(OGLES_GPGPU_OPENGL_ES3)
#  include "pbo.h"
#endif
// clang-format on

using namespace ogles_gpgpu;

#pragma mark static methods

bool MemTransfer::initPlatformOptimizations() {
    // always return false here. this method is only fully implemented
    // in platform-specialized classes of MemTransfer.
    return false;
}

#pragma mark constructor/deconstructor

MemTransfer::MemTransfer() {
    // set defaults
    inputW = inputH = outputW = outputH = 0;
    inputTexId = 0;
    outputTexId = 0;
    initialized = false;
    preparedInput = false;
    preparedOutput = false;
    inputPixelFormat = outputPixelFormat = OGLES_GPGPU_TEXTURE_FORMAT;

#if defined(OGLES_GPGPU_OPENGL_ES3)
    pboReaders.resize(1);
#endif
}

MemTransfer::~MemTransfer() {
    // release in- and output
    releaseInput();
    releaseOutput();
}

/**
 * Create N framebuffers for asynchronous downloads.
 * (Only supported for >= OpenGL ES 3.0)
 */
void MemTransfer::resizePBO(int count) {
#if defined(OGLES_GPGPU_OPENGL_ES3)
    pboReaders.resize(count);
#endif
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
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, inputW, inputH, 0, inputPixelFormat, GL_UNSIGNED_BYTE, nullptr);
#endif
    glBindTexture(GL_TEXTURE_2D, 0);

#if defined(OGLES_GPGPU_OPENGL_ES3)
    // ::::::: allocate ::::::::::
    pboWrite = std::unique_ptr<OPBO>(new OPBO(inputW, inputH));
#endif // defined(OGLES_GPGPU_OPENGL_ES3)

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

    GLenum rgbFormat = OGLES_GPGPU_TEXTURE_FORMAT;

    // create empty texture space on GPU
    glTexImage2D(GL_TEXTURE_2D, 0,
        GL_RGBA,
        outTexW, outTexH, 0,
        rgbFormat, GL_UNSIGNED_BYTE,
        NULL); // we do not need to pass texture data -> it will be generated!

    Tools::checkGLErr("MemTransfer", "fbo texture creation");

#if defined(OGLES_GPGPU_OPENGL_ES3)
    // ::::::: allocate ::::::::::
    for (auto& pbo : pboReaders) {
        pbo = std::unique_ptr<IPBO>(new IPBO(outputW, outputH));
    }
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
    if (pboWrite) {
        pboWrite.reset();
    }
#endif // defined(OGLES_GPGPU_OPENGL_ES3)
}

void MemTransfer::releaseOutput() {
    if (outputTexId > 0) {
        glDeleteTextures(1, &outputTexId);
        outputTexId = 0;
    }

#if defined(OGLES_GPGPU_OPENGL_ES3)
    if (pboReaders.size()) {
        for (auto& pbo : pboReaders) {
            pbo = nullptr;
        }
    }
#endif // defined(OGLES_GPGPU_OPENGL_ES3)
}

void MemTransfer::toGPU(const unsigned char* buf) {
    assert(preparedInput && inputTexId > 0 && buf);

#if defined(OGLES_GPGPU_OPENGL_ES3)

    pboWrite->bind();
    Tools::checkGLErr("MemTransfer", "toGPU (PBO::bind())");

    pboWrite->write(buf, inputTexId);
    Tools::checkGLErr("MemTransfer", "toGPU (PBO::write())");

    pboWrite->unbind();
    Tools::checkGLErr("MemTransfer", "toGPU (PBO::unbind())");

#else // defined(OGLES_GPGPU_OPENGL_ES3)
    // set input texture
    glBindTexture(GL_TEXTURE_2D, inputTexId); // bind input texture

    // copy data as texture to GPU (tested: OS X)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, inputW, inputH, 0, inputPixelFormat, GL_UNSIGNED_BYTE, buf);

    // check for error
    Tools::checkGLErr("MemTransfer", "toGPU (glTexImage2D)");
#endif // defined(OGLES_GPGPU_OPENGL_ES3)

    setCommonTextureParams(0);
}

void MemTransfer::fromGPU(unsigned char* buf, int index) {
    assert(preparedOutput && outputTexId);

#if defined(OGLES_GPGPU_OPENGL_ES3)
    assert(index < pboReaders.size());

    pboReaders[index]->bind();
    Tools::checkGLErr("MemTransfer", "toGPU (PBO::bind())");

    if (buf) {
        if (pboReaders[index]->isReadingAsynchronously()) {
            pboReaders[index]->finish(buf);
        } else {
            pboReaders[index]->read(buf);
        }
    } else {
        pboReaders[index]->start();
    }

    Tools::checkGLErr("MemTransfer", "toGPU (PBO::read())");

    pboReaders[index]->unbind();
    Tools::checkGLErr("MemTransfer", "toGPU (PBO::unbind())");

#else // defined(OGLES_GPGPU_OPENGL_ES3)
    assert(buf);
    glBindTexture(GL_TEXTURE_2D, outputTexId);
    Tools::checkGLErr("MemTransfer", "fromGPU: (glBindTexture)");

    // default (and slow) way using glReadPixels:
    glReadPixels(0, 0, outputW, outputH, outputPixelFormat, GL_UNSIGNED_BYTE, buf);
    Tools::checkGLErr("MemTransfer", "fromGPU: (glReadPixels)");
#endif // defined(OGLES_GPGPU_OPENGL_ES3)
}

// The zero copy fromGPU() call is not possibly with generic glReadPixels() access
void MemTransfer::fromGPU(const FrameDelegate& delegate, int index) {
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
        glBindTexture(target, texId);
        Tools::checkGLErr("MemTransfer", "setCommonTextureParams (glBindTexture)");
    }

    // set clamping (allows NPOT textures)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}
