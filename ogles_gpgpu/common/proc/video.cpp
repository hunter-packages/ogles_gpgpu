//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// See LICENSE file in project repository root for the license.
//

// Copyright (c) 2016-2017, David Hirvonen (this file)

#include "video.h"
#include "yuv2rgb.h"
#include <ogles_gpgpu/common/gl/memtransfer_optimized.h>

#if defined(OGLES_GPGPU_OPENGL_ES3)
#  define OGLES_GPGPU_BIPLANAR_CHANNEL_KIND Yuv2RgbProc::kRG
#else
#  define OGLES_GPGPU_BIPLANAR_CHANNEL_KIND Yuv2RgbProc::kLA
#endif

using namespace ogles_gpgpu;

VideoSource::VideoSource(void* glContext) {
    init(glContext);
}

VideoSource::VideoSource(void* glContext, const Size2d& size, GLenum inputPixFormat) {
    init(glContext);
    configurePipeline(size, inputPixFormat);
}

VideoSource::~VideoSource() {
    Core::getInstance()->reset();
}

void VideoSource::init(void* glContext) {
    auto* gpgpu = Core::getInstance();
    Core::getInstance()->setUseMipmaps(false); // TODO
    // pipeline
    gpgpu->init(glContext);
}

VideoSource::VideoSource(const Size2d& size, GLenum inputPixFormat) {
    configurePipeline(size, inputPixFormat);
}

GLuint VideoSource::getInputTexId() {
    assert(pipeline);
    return pipeline->getInputTexId();
}

void VideoSource::configurePipeline(const Size2d& size, GLenum inputPixFormat) {
    if (inputPixFormat == 0) { // 0 == NV{12,21}
        if (!yuv2RgbProc) {
            yuv2RgbProc = std::make_shared<ogles_gpgpu::Yuv2RgbProc>(Yuv2RgbProc::k601VideoRange, OGLES_GPGPU_BIPLANAR_CHANNEL_KIND);
            yuv2RgbProc->setExternalInputDataFormat(inputPixFormat);
            yuv2RgbProc->init(size.width, size.height, 0, true);
            frameSize = size;
        }

        if (size != frameSize) {
            yuv2RgbProc->reinit(size.width, size.height, true);
        }

        yuv2RgbProc->createFBOTex(false); // TODO: mipmapping?
    }

    if (pipeline != nullptr) {
        pipeline->prepare(size.width, size.height, inputPixFormat);
    }
    frameSize = size;
}

void VideoSource::set(ProcInterface* p) {
    pipeline = p;
}

void VideoSource::operator()(const FrameInput& frame) {
    return (*this)(frame.size, frame.pixelBuffer, frame.useRawPixels, frame.inputTexture, frame.textureFormat);
}

void VideoSource::configure(const Size2d& size, GLenum inputPixFormat)
{
    if (firstFrame || size != frameSize)
    {
        configurePipeline(size, inputPixFormat);
        firstFrame = false;
    }
}

void VideoSource::operator()(const Size2d& size, void* pixelBuffer, bool useRawPixels, GLuint inputTexture, GLenum inputPixFormat) {
    preConfig();

    if (m_timer)
        m_timer("begin");

    assert(pipeline);

    configure(size, inputPixFormat);

    if (firstFrame || size != frameSize) {
        configurePipeline(size, inputPixFormat);
        firstFrame = false;
    }

    auto gpgpuInputHandler = pipeline->getInputMemTransferObj();
    gpgpuInputHandler->setUseRawPixels(useRawPixels);

    // on each new frame, this will release the input buffers and textures, and prepare new ones
    // texture format must be GL_BGRA because this is one of the native camera formats (see initCam)
    if (pixelBuffer) {
        if (inputPixFormat == 0) {
            // YUV: Special case NV12=>BGR
            auto manager = yuv2RgbProc->getMemTransferObj();
            if (useRawPixels) {
                manager->setUseRawPixels(true);
            }
            manager->prepareInput(frameSize.width, frameSize.height, inputPixFormat, pixelBuffer);

            yuv2RgbProc->setTextures(manager->getLuminanceTexId(), manager->getChrominanceTexId());
            yuv2RgbProc->render();

            gpgpuInputHandler->prepareInput(frameSize.width, frameSize.height, GL_NONE, nullptr);
            inputTexture = yuv2RgbProc->getOutputTexId(); // override input parameter
        } else {
            gpgpuInputHandler->prepareInput(frameSize.width, frameSize.height, inputPixFormat, pixelBuffer);

            // For generic platforms we must also load pixel buffer to the texture:
            if (dynamic_cast<ogles_gpgpu::MemTransferOptimized*>(gpgpuInputHandler) == nullptr) {
                setInputData(reinterpret_cast<const unsigned char*>(pixelBuffer));
            }
            
            inputTexture = gpgpuInputHandler->getInputTexId(); // override input parameter
        }
    }

    if (m_timer)
        m_timer("process");

    assert(inputTexture); // inputTexture must be defined at this point
    pipeline->process(inputTexture, 1, GL_TEXTURE_2D, 0, 0, m_timer);

    if (m_timer)
        m_timer("end");

    postConfig();
}

void VideoSource::setInputData(const unsigned char* data) {

#if 1
    bool useMipmaps = false;
#else
    assert(initialized && inputTexId > 0);

    // check set up and input data
    if (useMipmaps && !inputSizeIsPOT && !glExtNPOTMipmaps) {
        OG_LOGINF("Core", "WARNING: NPOT input image provided but NPOT mipmapping not supported!");
        OG_LOGINF("Core", "mipmapping disabled!");
        useMipmaps = false;
    }
#endif

    // set texture
    glActiveTexture(GL_TEXTURE1);

    // copy data as texture to GPU
    pipeline->setExternalInputData(data);

    // mipmapping
    if (pipeline->getWillDownscale() && useMipmaps) {
        OG_LOGINF("Core", "generating mipmap for input image");
        // enabled
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        // disabled
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    Tools::checkGLErr("Core", "set texture parameters for input data");

    //glFinish();
}
