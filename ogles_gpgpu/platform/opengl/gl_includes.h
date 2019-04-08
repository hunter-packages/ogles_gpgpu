//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// Author: Markus Konrad <post@mkonrad.net>, Winter 2014/2015
// http://www.mkonrad.net
//
// See LICENSE file in project repository root for the license.
//

/**
 * OpenGL (not iOS or Android) : handle all
 */

#ifndef OGLES_GPGPU_OPENGL_GL_INCLUDES
#define OGLES_GPGPU_OPENGL_GL_INCLUDES

// clang-format off

#include "ogles_gpgpu/platform/opengl/gl_definitions.h"

//define something for Windows (64-bit)
#if defined(_WIN32) || defined(_WIN64)
#  include <algorithm> // min/max
#  include <windows.h> // CMakeLists.txt defines NOMINMAX
#  include <gl/glew.h>
#  include <GL/gl.h>
#elif __APPLE__
#  include "TargetConditionals.h"
#  if (TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR) || TARGET_OS_IPHONE
#    if defined(OGLES_GPGPU_OPENGL_ES3)
#      include <OpenGLES/ES3/gl.h>
#      include <OpenGLES/ES3/glext.h>
#    else
#      include <OpenGLES/ES2/gl.h>
#      include <OpenGLES/ES2/glext.h>
#    endif
#  else
#    if defined(OGLES_GPGPU_OPENGL_ES3)
#      include <OpenGL/gl3.h>
#      include <OpenGL/gl3ext.h>
#    else
#      include <OpenGL/gl.h>
#      include <OpenGL/glext.h>
#    endif
#  endif
#elif defined(__ANDROID__) || defined(ANDROID)
#  if defined(OGLES_GPGPU_OPENGL_ES3)
// ::: https://stackoverflow.com/q/31003863 :::
// According to the Khronos OpenGL ES Registry, the extension header for 
// GLES 3.0 is actually <GLES2/gl2ext.h>.  gl3ext.h should be empty and 
// provided only for legacy compatibility. Thus, if you want to include 
// GLES 3.0 headers, you should do:
//
// ::: https://stackoverflow.com/a/31025110 :::
// ...this is fixed in API-21. However, if you still need 
// to use API-18 or API-19, there is a work-around. You can simply:
// [define __gl2_h_] when gl2ext.h includes gl2.h, the defined include 
// guard will cause the contents of gl2.h to be skipped.
#     if __ANDROID_API__ < 21
#       define __gl2_h_
#     endif
#    include <GLES3/gl3.h>
#    include <GLES2/gl2ext.h>  // GL_TEXTURE_EXTERNAL_OES, etc
#  else
#    include <GLES2/gl2.h>
#    include <GLES2/gl2ext.h>
#  endif
#elif defined(__linux__) || defined(__unix__) || defined(__posix__)
#  define OGLES_GPGPU_NIX 1
#  define GL_GLEXT_PROTOTYPES 1
#  if defined(OGLES_GPGPU_OPENGL_ES2)
#    include <GLES2/gl2.h>
#    include <GLES2/gl2ext.h>
#  elif defined(OGLES_GPGPU_OPENGL_ES3)
#    include <GLES3/gl3.h>
#    include <GLES3/gl3ext.h>
#  else
#    include <GL/gl.h>
#    include <GL/glext.h>
#  endif
#else
#  error platform not supported.
#endif
// clang-format on

// clang-format off
#if defined(ANDROID) || defined(OGLES_GPGPU_NIX)
#  define OGLES_GPGPU_TEXTURE_FORMAT GL_RGBA
#  define OGLES_GPGPU_RGBA_FORMAT 1
#else
#  define OGLES_GPGPU_TEXTURE_FORMAT GL_BGRA
#  define OGLES_GPGPU_RGBA_FORMAT 0
#endif

#if defined(OGLES_GPGPU_OPENGL_ES3)
#  define OGLES_GPGPU_PLANAR_INTERNAL_FORMAT GL_R8
#  define OGLES_GPGPU_PLANAR_FORMAT GL_RED
#  define OGLES_GPGPU_BIPLANAR_INTERNAL_FORMAT GL_RG8
#  define OGLES_GPGPU_BIPLANAR_FORMAT GL_RG
#else
#  define OGLES_GPGPU_PLANAR_INTERNAL_FORMAT GL_LUMINANCE
#  define OGLES_GPGPU_PLANAR_FORMAT GL_LUMINANCE
#  define OGLES_GPGPU_BIPLANAR_INTERNAL_FORMAT GL_LUMINANCE_ALPHA
#  define OGLES_GPGPU_BIPLANAR_FORMAT GL_LUMINANCE_ALPHA
#endif
// clang-format off

#endif // OGLES_GPGPU_OPENGL_GL_INCLUDES
