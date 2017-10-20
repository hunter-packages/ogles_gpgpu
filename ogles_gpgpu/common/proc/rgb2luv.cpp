//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// See LICENSE file in project repository root for the license.
//

// Copyright (c) 2014-2017, David Hirvonen (this file)

#include "rgb2luv.h"

BEGIN_OGLES_GPGPU

// clang-format off
const char * Rgb2LuvProc::fshaderRgb2LuvSrc = 
#if defined(OGLES_GPGPU_OPENGLES)
  precision highp float;
#endif
OG_TO_STR(
  varying vec2 vTexCoord;
  uniform sampler2D uInputTex;

  const mat3 RGBtoXYZ = mat3(0.430574, 0.341550, 0.178325, 0.222015, 0.706655, 0.071330, 0.020183, 0.129553, 0.93918);

  const float y0 = 0.00885645167; // pow(6.0/29.0, 3.0)
  const float a = 903.296296296;  // pow(29.0/3.0, 3.0);
  const float un = 0.197833;
  const float vn = 0.468331;
  const float maxi = 0.0037037037;  // 1.0/270.0;
  const float minu = -88.0 * maxi;
  const float minv = -134.0 * maxi;
  const vec3 k = vec3(1.0, 15.0, 3.0);
  const vec3 RGBtoGray = vec3(0.2125, 0.7154, 0.0721);

  void main()
  {
      vec3 rgb = texture2D(uInputTex, vTexCoord).rgb;
      vec3 xyz = (rgb * RGBtoXYZ);
      float z = 1.0/(dot(xyz,k) + 1e-35);

      vec3 luv;
      float y = xyz.y;
      float l = ((y > y0) ? ((116.0 * pow(y, 0.3333333333)) - 16.0) : (y*a)) * maxi;
      luv.x = l;
      luv.y = l * ((52.0 * xyz.x * z) - (13.0*un)) - minu;
      luv.z = l * ((117.0 * xyz.y * z) - (13.0*vn)) - minv;

      gl_FragColor = vec4(vec3(luv.xyz), dot(rgb, RGBtoGray));
  }
);
// clang-format on

END_OGLES_GPGPU
