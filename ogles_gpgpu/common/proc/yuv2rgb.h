//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// See LICENSE file in project repository root for the license.
//

// Copyright (c) 2016-2017, David Hirvonen (this file)

/**
 * GPGPU yuv2rgb processor.
 */
#ifndef OGLES_GPGPU_COMMON_PROC_YUV2RGB
#define OGLES_GPGPU_COMMON_PROC_YUV2RGB

#include "../common_includes.h"

#include "base/filterprocbase.h"

BEGIN_OGLES_GPGPU

/**
 * GPGPU yuv2rgb processor will perform yuv to rgb colorspace transformation
 */
class Yuv2RgbProc : public FilterProcBase {
public:
    enum ChannelKind {
        kLA,       // for interleaved NV21 typically ES 2.0
        kRG,       // for interleaved NV21 typically ES 3.0
        kYUV12     // for planar YUV12
    };

    enum YUVKind {
        k601VideoRange,
        k601FullRange,
        k709Default
    };

    /**
     * Constructor.
     */
    Yuv2RgbProc(YUVKind yuvKind = k601VideoRange, ChannelKind channelKind = kLA);

    /**
     * Return the processors name.
     */
    virtual const char* getProcName() {
        return "Yuv2RgbProc";
    }

    /**
     * Init the processor for input frames of size <inW>x<inH> which is at
     * position <order> in the processing pipeline.
     */
    virtual int init(int inW, int inH, unsigned int order, bool prepareForExternalInput = false);

    /**
     * Render the output.
     */
    virtual int render(int position = 0);

    /**
     * Create the shader program
     */
    virtual void filterShaderSetup(const char* vShaderSrc, const char* fShaderSrc, GLenum target);

    /**
     * Set input (planar) luminance and (interleaved) chrominance textures (NV12).
     */
    void setTextures(GLuint luminanceTexture, GLuint chrominanceTexture);
    
    /**
     * Set input planar luminance and planr U and V textures (YUV12).
     */
    void setTextures(GLuint luminanceTexture, GLuint uTexture, GLuint vTexture);

private:
    virtual void filterRenderPrepare();

    static const char* vshaderYuv2RgbSrc; // fragment shader source
    static const char* fshaderYuv2RgbSrc; // fragment shader source

    GLuint luminanceTexture;

    // For (interleaved) NV{12,21}
    GLuint chrominanceTexture;
    GLint yuvConversionChrominanceTextureUniform;
    
    // For (planar) YUV12
    GLuint uTexture;
    GLint yuvConversionUTextureUniform;
    GLuint vTexture;
    GLint yuvConversionVTextureUniform;

    GLint yuvConversionPositionAttribute;
    GLint yuvConversionTextureCoordinateAttribute;

    GLint yuvConversionLuminanceTextureUniform;
    GLint yuvConversionMatrixUniform;

    const GLfloat* _preferredConversion;

    YUVKind yuvKind = k601VideoRange;
    ChannelKind channelKind = kLA;
};

END_OGLES_GPGPU

#endif
