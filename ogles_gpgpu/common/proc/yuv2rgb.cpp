//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// See LICENSE file in project repository root for the license.
//

// Original shaders: https://github.com/BradLarson/GPUImage/blob/master/framework/Source/GPUImageVideoCamera.m
// Modifications: Copyright (c) 2016-2017, David Hirvonen (this file)

#include "yuv2rgb.h"
#include "../common_includes.h"

using namespace std;
using namespace ogles_gpgpu;

// Color Conversion Constants (YUV to RGB) including adjustment from 16-235/16-240 (video range)

// BT.601, which is the standard for SDTV.
GLfloat kColorConversion601Default[] = {
    1.164, 1.164, 1.164,
    0.0, -0.392, 2.017,
    1.596, -0.813, 0.0,
};

// BT.601 full range (ref: http://www.equasys.de/colorconversion.html)
GLfloat kColorConversion601FullRangeDefault[] = {
    1.0, 1.0, 1.0,
    0.0, -0.343, 1.765,
    1.4, -0.711, 0.0,
};

// BT.709, which is the standard for HDTV.
GLfloat kColorConversion709Default[] = {
    1.164, 1.164, 1.164,
    0.0, -0.213, 2.112,
    1.793, -0.533, 0.0,
};

GLfloat* kColorConversion601 = kColorConversion601Default;
GLfloat* kColorConversion601FullRange = kColorConversion601FullRangeDefault;
GLfloat* kColorConversion709 = kColorConversion709Default;

// clang-format off
const char *kGPUImageYUVFullRangeConversionForRGFragmentShaderString =
#if defined(OGLES_GPGPU_OPENGLES)
OG_TO_STR(precision mediump float;)
#endif
OG_TO_STR(
          
  varying vec2 vTexCoord;
  
  uniform sampler2D luminanceTexture;
  uniform sampler2D chrominanceTexture;
  uniform mat3 colorConversionMatrix;
  
  void main()
  {
      vec3 yuv;
      vec3 rgb;
      
      yuv.x = texture2D(luminanceTexture, vTexCoord).r;
      yuv.yz = texture2D(chrominanceTexture, vTexCoord).rg - vec2(0.5, 0.5);
      rgb = colorConversionMatrix * yuv;
      
      gl_FragColor = vec4(rgb, 1);
  });
// clang-format on

// clang-format off
const char *kGPUImageYUVVideoRangeConversionForRGFragmentShaderString =
#if defined(OGLES_GPGPU_OPENGLES)
OG_TO_STR(precision mediump float;)
#endif
OG_TO_STR(
          
  varying vec2 vTexCoord;
  
  uniform sampler2D luminanceTexture;
  uniform sampler2D chrominanceTexture;
  uniform mat3 colorConversionMatrix;
  
  void main()
  {
      vec3 yuv;
      vec3 rgb;
      
      yuv.x = texture2D(luminanceTexture, vTexCoord).r - (16.0/255.0);
      yuv.yz = texture2D(chrominanceTexture, vTexCoord).rg - vec2(0.5, 0.5);
      rgb = colorConversionMatrix * yuv;
      
      gl_FragColor = vec4(rgb, 1);
  });
// clang-format on

// clang-format off
const char *kGPUImageYUVFullRangeConversionForLAFragmentShaderString =
#if defined(OGLES_GPGPU_OPENGLES)
OG_TO_STR(precision mediump float;)
#endif
OG_TO_STR(

 varying vec2 vTexCoord;

 uniform sampler2D luminanceTexture;
 uniform sampler2D chrominanceTexture;
 uniform mat3 colorConversionMatrix;

 void main()
 {
     vec3 yuv;
     vec3 rgb;

     yuv.x = texture2D(luminanceTexture, vTexCoord).r;
     yuv.yz = texture2D(chrominanceTexture, vTexCoord).ra - vec2(0.5, 0.5);
     rgb = colorConversionMatrix * yuv;

     gl_FragColor = vec4(rgb, 1);
 });
// clang-format on

// clang-format off
const char *kGPUImageYUVVideoRangeConversionForLAFragmentShaderString =
#if defined(OGLES_GPGPU_OPENGLES)
OG_TO_STR(precision mediump float;)
#endif
OG_TO_STR(
                                                                                  
 varying vec2 vTexCoord;

 uniform sampler2D luminanceTexture;
 uniform sampler2D chrominanceTexture;
 uniform mat3 colorConversionMatrix;

 void main()
 {
     vec3 yuv;
     vec3 rgb;

     yuv.x = texture2D(luminanceTexture, vTexCoord).r - (16.0/255.0);
     yuv.yz = texture2D(chrominanceTexture, vTexCoord).ra - vec2(0.5, 0.5);
     rgb = colorConversionMatrix * yuv;

     gl_FragColor = vec4(rgb, 1);          
 });
// clang-format on

// ----YUV12----

// clang-format off
const char *kGPUImageYUVVideoRangeConversionForYUV12FragmentShaderString =
#if defined(OGLES_GPGPU_OPENGLES)
OG_TO_STR(precision mediump float;)
#endif
OG_TO_STR(
                                                                                  
 varying vec2 vTexCoord;

 uniform sampler2D luminanceTexture;
 uniform sampler2D uTexture;
 uniform sampler2D vTexture; 
 uniform mat3 colorConversionMatrix;

 void main()
 {
     vec3 yuv;
     vec3 rgb;

     yuv.x = texture2D(luminanceTexture, vTexCoord).r - (16.0/255.0);
     yuv.y = texture2D(uTexture, vTexCoord).r - 0.5;
     yuv.z = texture2D(vTexture, vTexCoord).r - 0.5;
     
     rgb = colorConversionMatrix * yuv;

     gl_FragColor = vec4(rgb, 1);          
 });
// clang-format on

// clang-format off
const char *kGPUImageYUVFullRangeConversionForYUV12FragmentShaderString =
#if defined(OGLES_GPGPU_OPENGLES)
OG_TO_STR(precision mediump float;)
#endif
OG_TO_STR(
                                                                                  
 varying vec2 vTexCoord;

 uniform sampler2D luminanceTexture;
 uniform sampler2D uTexture;
 uniform sampler2D vTexture; 
 uniform mat3 colorConversionMatrix;

 void main()
 {
     vec3 yuv;
     vec3 rgb;

     yuv.x = texture2D(luminanceTexture, vTexCoord).r;
     yuv.y = texture2D(uTexture, vTexCoord).r - 0.5;
     yuv.z = texture2D(vTexture, vTexCoord).r - 0.5;
     rgb = colorConversionMatrix * yuv;

     gl_FragColor = vec4(rgb, 1);          
 });
// clang-format on

// =================================================================================

Yuv2RgbProc::Yuv2RgbProc(YUVKind yuvKind, ChannelKind channelKind)
    : yuvKind(yuvKind)
    , channelKind(channelKind) {
    switch (yuvKind) {
    case k601VideoRange:
        _preferredConversion = kColorConversion601;
        break;
    case k601FullRange:
        _preferredConversion = kColorConversion601FullRange;
        break;
    case k709Default:
        _preferredConversion = kColorConversion709;
        break;
    }

    texTarget = GL_TEXTURE_2D;
}

void Yuv2RgbProc::setTextures(GLuint luminance, GLuint chrominance) {
    luminanceTexture = luminance;
    chrominanceTexture = chrominance;
}

void Yuv2RgbProc::setTextures(GLuint luminance, GLuint u, GLuint v) {
    luminanceTexture = luminance;
    uTexture = u;
    vTexture = v;
}

void Yuv2RgbProc::filterShaderSetup(const char* vShaderSrc, const char* fShaderSrc, GLenum target) {

    ProcBase::createShader(vShaderSrc, fShaderSrc, target);
    Tools::checkGLErr(getProcName(), "createShader()");

    // get shader params
    shParamAPos = shader->getParam(ATTR, "aPos");
    shParamATexCoord = shader->getParam(ATTR, "aTexCoord");
    yuvConversionPositionAttribute = shParamAPos; // "aPos"
    yuvConversionTextureCoordinateAttribute = shParamATexCoord; // "aTexCoord"
    yuvConversionLuminanceTextureUniform = shader->getParam(UNIF, "luminanceTexture");
    
    if(channelKind == kYUV12) {
        yuvConversionUTextureUniform = shader->getParam(UNIF, "uTexture");
        yuvConversionVTextureUniform = shader->getParam(UNIF, "vTexture");
    }
    else {
        yuvConversionChrominanceTextureUniform = shader->getParam(UNIF, "chrominanceTexture");
    }
    
    yuvConversionMatrixUniform = shader->getParam(UNIF, "colorConversionMatrix");
    Tools::checkGLErr(getProcName(), "getParam()");

    // remember used shader source
    vertexShaderSrcForCompilation = vShaderSrc;
    fragShaderSrcForCompilation = fShaderSrc;
}

int Yuv2RgbProc::init(int inW, int inH, unsigned int order, bool prepareForExternalInput) {
    OG_LOGINF(getProcName(), "initialize");

    // create fbo for output
    createFBO();

    // ProcBase init - set defaults
    baseInit(inW, inH, order, prepareForExternalInput, procParamOutW, procParamOutH, procParamOutScale);

    // FilterProcBase init - create shaders, get shader params, set buffers for OpenGL

    switch (channelKind) {
    case kRG:
        switch (yuvKind) {
        case k601VideoRange:
            filterInit(FilterProcBase::vshaderDefault, kGPUImageYUVVideoRangeConversionForRGFragmentShaderString);
            break;
        case k601FullRange:
        case k709Default:
            filterInit(FilterProcBase::vshaderDefault, kGPUImageYUVFullRangeConversionForRGFragmentShaderString);
            break;
        }
        break;

    case kLA:
        switch (yuvKind) {
        case k601VideoRange:
            filterInit(FilterProcBase::vshaderDefault, kGPUImageYUVVideoRangeConversionForLAFragmentShaderString);
            break;
        case k601FullRange:
        case k709Default:
            filterInit(FilterProcBase::vshaderDefault, kGPUImageYUVFullRangeConversionForLAFragmentShaderString);
            break;
        }
        break;
    case kYUV12:
        switch (yuvKind) {
            case k601VideoRange:
                filterInit(FilterProcBase::vshaderDefault, kGPUImageYUVVideoRangeConversionForYUV12FragmentShaderString);
                break;
            case k601FullRange:
            case k709Default:
                filterInit(FilterProcBase::vshaderDefault, kGPUImageYUVFullRangeConversionForYUV12FragmentShaderString);
                break;
        }
        break;
    }

    return 1;
}

void Yuv2RgbProc::filterRenderPrepare() {
    shader->use();

    // set the viewport
    glViewport(0, 0, outFrameW, outFrameH);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, luminanceTexture);
    glUniform1i(yuvConversionLuminanceTextureUniform, 4);

    if(channelKind >= kYUV12)
    {
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, uTexture);
        glUniform1i(yuvConversionUTextureUniform, 5);

        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, vTexture);
        glUniform1i(yuvConversionVTextureUniform, 6);
    }
    else
    {
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, chrominanceTexture);
        glUniform1i(yuvConversionChrominanceTextureUniform, 5);
    }

    glUniformMatrix3fv(yuvConversionMatrixUniform, 1, GL_FALSE, _preferredConversion);
}

int Yuv2RgbProc::render(int position) {
    OG_LOGINF(getProcName(), "input tex %d, target %d, framebuffer of size %dx%d", texId, texTarget, outFrameW, outFrameH);

    filterRenderPrepare();
    Tools::checkGLErr(getProcName(), "render prepare");

    filterRenderSetCoords();
    Tools::checkGLErr(getProcName(), "render set coords");

    filterRenderDraw();
    Tools::checkGLErr(getProcName(), "render draw");

    filterRenderCleanup();
    Tools::checkGLErr(getProcName(), "render cleanup");

    return 0;
}
