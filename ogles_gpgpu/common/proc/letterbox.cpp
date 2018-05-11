//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// See LICENSE file in project repository root for the license.
//

// Copyright (c) 2016-2017, David Hirvonen (this file)

#include "letterbox.h"

BEGIN_OGLES_GPGPU

// clang-format off
const char *LetterboxProc::fshaderLetterboxSrc =
#if defined(OGLES_GPGPU_OPENGLES)
OG_TO_STR(precision highp float;)
#endif
OG_TO_STR(
 varying vec2 vTexCoord;
 uniform sampler2D uInputTex;

 uniform float height;
 uniform vec3 color;

void main()
{
    vec4 pixel = vec4(texture2D(uInputTex, vTexCoord).rgba);
    vec3 mixed = mix(pixel.rgb, color, step(height*0.5, abs(vTexCoord.y-0.5)));
    gl_FragColor = vec4(mixed, 1.0);
}
);
// clang-format on

LetterboxProc::LetterboxProc(float height)
    : height(height)
    , color({ { 1.f, 1.f, 1.f } }) {
}


void LetterboxProc::filterRenderDraw() {
    FilterProcBase::filterRenderDraw();
    if (func) {
        func(this);
    }
}

void LetterboxProc::getUniforms() {
    shParamULetterboxHeight = shader->getParam(UNIF, "height");
    shParamULetterboxColor = shader->getParam(UNIF, "color");
}

void LetterboxProc::setUniforms() {
    glUniform1f(shParamULetterboxHeight, height);
    glUniform3f(shParamULetterboxColor, color[0], color[1], color[2]);
}

END_OGLES_GPGPU
