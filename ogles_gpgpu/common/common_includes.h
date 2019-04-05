//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// Author: Markus Konrad <post@mkonrad.net>, Winter 2014/2015
// http://www.mkonrad.net
//
// See LICENSE file in project repository root for the license.
//

/**
 * Common header includes
 */

#include <cassert>
#include <iostream>

// Need initial preprocessor pass for nested includes

// clang-format off
#if __APPLE__
#  include "TargetConditionals.h"
#  if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
#    define OGLES_GPGPU_IOS 1
#  else
#    define OGLES_GPGPU_OSX 1
#  endif
#elif __ANDROID__ || defined(ANDROID)
#  define OGLES_GPGPU_ANDROID 1
#elif defined(_WIN32) || defined(_WIN64)
#  define OGLES_GPGPU_WINDOWS 1
#endif

#include "../platform/opengl/gl_includes.h"

#if defined(OGLES_GPGPU_IOS)
#  include "macros.h"
#elif defined(OGLES_GPGPU_ANDROID) && !defined(OGLES_GPGPU_OPENGL_ES3)
// NOTE: The GraphicsBuffer acceleration tricks don't work in recent
// Android versions and are only relevant in OpenGL ES 2.0
#  include "../platform/android/gl_includes.h"
#  include "../platform/android/macros.h"
#  include "../platform/android/egl.h"
#else
#  include "../platform/opengl/gl_includes.h"
#  include "macros.h"
#endif
// clang-format on

// clang-format off
#if defined(OGLES_GPGPU_OPENGL_ES2) || defined(OGLES_GPGPU_OPENGL_ES3)
#  define OGLES_GPGPU_OPENGLES 1
#else
#  define OGLES_GPGPU_OPENGL 1
#endif
// clang-format on

#define BEGIN_OGLES_GPGPU namespace ogles_gpgpu {
#define END_OGLES_GPGPU }

#include "tools.h"
#include "types.h"
