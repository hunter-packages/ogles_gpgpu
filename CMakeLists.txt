# Copyright (c) 2015-2017, David Hirvonen, Ruslan Baratov
# All rights reserved.

cmake_minimum_required(VERSION 3.0)

include("cmake/hunter_cache_update.cmake")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

include("cmake/HunterGate.cmake")
HunterGate(
  URL "https://github.com/ruslo/hunter/archive/v0.23.151.tar.gz"
  SHA1 "68657b81508c2d3c248731b5a0c2125f19866721"
  LOCAL
  )

project(ogles_gpgpu VERSION 0.3.7)

# !!! Make sure option OGLES_GPGPU_OPENG_ES3 occurs prior to the first
# hunter_add_package() call.  This will allow us to modify settings
# in dependencies appropriately (see cmake/Hunter/config.cmake)

if(IOS OR ANDROID)
  set(ogles_gpgpu_opengl_es2_dflt ON)
  set(ogles_gpgpu_opengl_es3_dflt OFF)
else()
  set(ogles_gpgpu_opengl_es2_dflt OFF)
  set(ogles_gpgpu_opengl_es3_dflt OFF)
endif()

option(OGLES_GPGPU_OPENGL_ES2 "Use OpenGL ES 2.0" ${ogles_gpgpu_opengl_es2_dflt})
option(OGLES_GPGPU_OPENGL_ES3 "Use OpenGL ES 3.0" ${ogles_gpgpu_opengl_es3_dflt})
option(OGLES_GPGPU_USE_EGL "Use EGL to manage the context (supports ES)" OFF)

if(OGLES_GPGPU_OPENGL_ES2 AND OGLES_GPGPU_OPENGL_ES3)
  message(FATAL_ERROR
    "OGLES_GPGPU_OPENGL_ES2 and OGLES_GPGPU_OPENGL_ES3 are mutually exclusive"
  )
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

string(COMPARE EQUAL "${CMAKE_SYSTEM_NAME}" "Linux" is_linux)

option(OGLES_GPGPU_BUILD_EXAMPLES "Build examples" OFF)
option(OGLES_GPGPU_INSTALL "Perform installation" ON)
option(OGLES_GPGPU_VERBOSE "Perform per filter logging" OFF)
option(OGLES_GPGPU_BUILD_TESTS "Build shader unit tests" OFF)
option(OGLES_GPGPU_USE_OSMESA "Use MESA CPU OpenGL (via glfw)" OFF)

hunter_add_package(check_ci_tag)
find_package(check_ci_tag CONFIG REQUIRED)
check_ci_tag()

# See: cmake/Hunter/config.cmake
hunter_add_package(sugar)
find_package(sugar CONFIG REQUIRED)
include(sugar_include)

message("Android: ${ANDROID}")
message("iOS    : ${IOS}")
message("Linux  : ${is_linux}")
message("Xcode  : ${XCODE}")

if(XCODE)
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configuration types")
endif()

## #################################################################
## Testing:
## #################################################################

string(COMPARE EQUAL "$ENV{TRAVIS}" "true" travis_ci)
string(COMPARE EQUAL "$ENV{APPVEYOR}" "True" appveyor_ci)
if(travis_ci OR appveyor_ci)
  set(OGLES_GPGPU_CI TRUE)
else()
  set(OGLES_GPGPU_CI FALSE)
endif()

if(ANDROID OR NOT ${OGLES_GPGPU_CI})
  set(OGLES_GPGPU_DO_GPU_TESTING TRUE)
else()
  set(OGLES_GPGPU_DO_GPU_TESTING FALSE)
endif()

# Tests can be compiled for all platforms (to tests exe linking),
# but they may only be run on platforms where an OpenGL context
# is available
if(OGLES_GPGPU_BUILD_TESTS)
  if(IOS AND DRISHTI_CI)
    # do not run test on CI (TODO: remote device testing)
  else()
    enable_testing()
  endif()
endif()

## #################################################################
## Project
## #################################################################

add_subdirectory(ogles_gpgpu)

## #################################################################
## Examples
## #################################################################

if(OGLES_GPGPU_BUILD_EXAMPLES)
  message("BUILD EXAMPLES.......")
  add_subdirectory(examples)
endif()
