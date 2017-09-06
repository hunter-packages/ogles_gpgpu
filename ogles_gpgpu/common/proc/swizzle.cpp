//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// See LICENSE file in project repository root for the license.
//

// Copyright (c) 2016-2017, David Hirvonen (this file)

#include "swizzle.h"

BEGIN_OGLES_GPGPU

// clang-format off
const char * SwizzleProc::fshaderRGBASrc = OG_TO_STR
(
#if defined(OGLES_GPGPU_OPENGLES)
 precision mediump float;
#endif
 varying vec2 vTexCoord;
 uniform sampler2D uInputTex;
 void main()
 {
     vec4 val = texture2D(uInputTex, vTexCoord);
     gl_FragColor = val.rgba;
 });
// clang-format on

// clang-format off
const char * SwizzleProc::fshaderBGRASrc = OG_TO_STR
(
#if defined(OGLES_GPGPU_OPENGLES)
 precision mediump float;
#endif
 varying vec2 vTexCoord;
 uniform sampler2D uInputTex;
 void main()
 {
     vec4 val = texture2D(uInputTex, vTexCoord);
     gl_FragColor = val.bgra;
 });
// clang-format on

// clang-format off
const char * SwizzleProc::fshaderARGBSrc = OG_TO_STR
(
#if defined(OGLES_GPGPU_OPENGLES)
 precision mediump float;
#endif
 varying vec2 vTexCoord;
 uniform sampler2D uInputTex;
 void main()
 {
     vec4 val = texture2D(uInputTex, vTexCoord);
     gl_FragColor = val.argb;
 });
// clang-format on

// clang-format off
const char * SwizzleProc::fshaderABGRSrc = OG_TO_STR
(
#if defined(OGLES_GPGPU_OPENGLES)
 precision mediump float;
#endif
 varying vec2 vTexCoord;
 uniform sampler2D uInputTex;
 void main()
 {
     vec4 val = texture2D(uInputTex, vTexCoord);
     gl_FragColor = val.abgr;
 });
// clang-format on

// clang-format off
const char * SwizzleProc::fshaderGRABSrc = OG_TO_STR
(
#if defined(OGLES_GPGPU_OPENGLES)
 precision mediump float;
#endif
 varying vec2 vTexCoord;
 uniform sampler2D uInputTex;
 void main()
 {
     vec4 val = texture2D(uInputTex, vTexCoord);
     gl_FragColor = val.grab;
 });
// clang-format on

const char* SwizzleProc::getFragmentShaderSource() {
    switch (swizzleKind) {
    case kSwizzleRGBA:
        return fshaderRGBASrc;
    case kSwizzleBGRA:
        return fshaderBGRASrc;
    case kSwizzleARGB:
        return fshaderARGBSrc;
    case kSwizzleABGR:
        return fshaderABGRSrc;
    case kSwizzleGRAB:
        return fshaderGRABSrc;
    }
}

END_OGLES_GPGPU
