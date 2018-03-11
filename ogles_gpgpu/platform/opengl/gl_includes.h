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
#      include <OpenGLES/ES2/glext2.h>
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
#    include <GLES3/gl3.h>
#    include <GLES3/gl3ext.h>
#  else
#    include <GLES2/gl2.h>
#    include <GLES2/gl2ext.h>
#  endif
#elif defined(__linux__) || defined(__unix__) || defined(__posix__)
#  define GL_GLEXT_PROTOTYPES 1
#  include <GL/gl.h>
#  include <GL/glext.h>
#else
#  error platform not supported.
#endif
// clang-format on

// clang-format off
#ifdef ANDROID
#  define OGLES_GPGPU_TEXTURE_FORMAT GL_RGBA
#else
#  define OGLES_GPGPU_TEXTURE_FORMAT GL_BGRA
#endif
// clang-format off

#endif // OGLES_GPGPU_OPENGL_GL_INCLUDES
