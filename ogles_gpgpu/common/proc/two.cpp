//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// See LICENSE file in project repository root for the license.
//

// Copyright (c) 2016-2017, David Hirvonen (this file)

#include "two.h"
#include "../common_includes.h"

using namespace std;
using namespace ogles_gpgpu;

// clang-format off
const char *TwoInputProc::vshaderTwoInputSrc = 

#if defined(OGLES_GPGPU_OPENGLES)
OG_TO_STR(precision mediump float;)
#endif
OG_TO_STR(
 attribute vec4 position;
 attribute vec4 inputTextureCoordinate;

 varying vec2 textureCoordinate;

 void main()
 {
     gl_Position = position;
     textureCoordinate = inputTextureCoordinate.xy;
 });
// clang-format on

// clang-format off
const char *TwoInputProc::fshaderTwoInputSrc = 

#if defined(OGLES_GPGPU_OPENGLES)
OG_TO_STR(precision mediump float;)
#endif
OG_TO_STR(
 varying vec2 textureCoordinate;

 uniform sampler2D inputImageTexture;
 uniform sampler2D inputImageTexture2;

 void main()
 {
	 vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);
	 vec4 textureColor2 = texture2D(inputImageTexture2, textureCoordinate);
	 gl_FragColor = vec4(textureColor.rgb - textureColor2.rgb, textureColor.a);
});
// clang-format on

TwoInputProc::TwoInputProc() {
}

int TwoInputProc::render(int position) {
    int result = 1;

    switch (position) {
    case 0:
        hasTex1 = true;
        break;
    case 1:
        hasTex2 = true;
        break;
    default:
        assert(false);
    }

    if (hasTex1 && (hasTex2 || !waitForSecondTexture)) {
        result = FilterProcBase::render(position);
        hasTex1 = hasTex2 = false;
    }
    return result; // 0 on success
}

/**
 * Use texture id <id> as input texture at texture <useTexUnit> with texture target <target>.
 */

void TwoInputProc::useTexture(GLuint id, GLuint useTexUnit, GLenum target, int position) {
    switch (position) {
    case 0:
        FilterProcBase::useTexture(id, useTexUnit, target, position);
        break;
    case 1:
        useTexture2(id, useTexUnit, target);
        break;
    default:
        assert(false);
    }
}

void TwoInputProc::useTexture2(GLuint id, GLuint useTexUnit, GLenum target) {
    texId2 = id;
    texUnit2 = useTexUnit;
    texTarget2 = target;
}

void TwoInputProc::filterRenderPrepare() {
    shader->use();

    // set the viewport
    glViewport(0, 0, outFrameW, outFrameH);

    // Bind input texture 1:
    glActiveTexture(GL_TEXTURE0 + texUnit);
    glBindTexture(texTarget, texId);
    glUniform1i(shParamUInputTex, texUnit);

    // Bind input texture 2:
    texUnit2 = texUnit + 1;
    glActiveTexture(GL_TEXTURE0 + texUnit2);
    glBindTexture(texTarget2, texId2);
    glUniform1i(shParamUInputTex2, texUnit2);
}

void TwoInputProc::filterShaderSetup(const char* vShaderSrc, const char* fShaderSrc, GLenum target) {
    // create shader object
    FilterProcBase::createShader(vShaderSrc, fShaderSrc, target);

    shParamAPos = shader->getParam(ATTR, "position");
    shParamATexCoord = shader->getParam(ATTR, "inputTextureCoordinate");

    // remember used shader source
    vertexShaderSrcForCompilation = vShaderSrc;
    fragShaderSrcForCompilation = fShaderSrc;
}

void TwoInputProc::getUniforms() {
    FilterProcBase::getUniforms();
    shParamUInputTex = shader->getParam(UNIF, "inputImageTexture");
    shParamUInputTex2 = shader->getParam(UNIF, "inputImageTexture2");
}

void TwoInputProc::setUniforms() {
    FilterProcBase::setUniforms();
}
