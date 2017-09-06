# Copyright (c) 2015-2017, David Hirvonen, Ruslan Baratov
# All rights reserved.

cmake_minimum_required(VERSION 3.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

include("cmake/HunterGate.cmake")
HunterGate(
  URL "https://github.com/ruslo/hunter/archive/v0.19.93.tar.gz"
  SHA1 "eaa24bdb953fad6249dcb9d7d6a310061ffdcede"
  LOCAL
)

project(ogles_gpgpu VERSION 0.2.1)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

string(COMPARE EQUAL "${CMAKE_SYSTEM_NAME}" "Linux" is_linux)

option(OGLES_GPGPU_BUILD_EXAMPLES "Build examples" OFF)
option(OGLES_GPGPU_INSTALL "Perform installation" ON)
option(OGLES_GPGPU_VERBOSE "Perform per filter logging" OFF)
option(OGLES_GPGPU_BUILD_TESTS "Build shader unit tests" OFF)
option(OGLES_GPGPU_USE_OSMESA "Use MESA CPU OpenGL (via glfw)" OFF)

# !!! Make sure option OGLES_GPGPU_OPENG_ES3 occurs prior to the first
# hunter_add_package() call.  This will allow us to modify settings
# in dependencies appropriately (see cmake/Hunter/config.cmake)
option(OGLES_GPGPU_OPENGL_ES3 "Use OpenGL ES 3.0" OFF)

# See: cmake/Hunter/config.cmake
hunter_add_package(Sugar)
include("${SUGAR_ROOT}/cmake/Sugar")
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

  # Use GTest as unit test framework
  hunter_add_package(GTest)
  find_package(GTest CONFIG REQUIRED)
  list(APPEND OGLES_GPGPU_TEST_LIBS GTest::gtest)

  if(MSVC)
    # TODO: Temporary aglet fix for appveyor?  Shouldn't be needed.
    hunter_add_package(glew)
    find_package(glew CONFIG REQUIRED)
  endif()

  # Use aglet for portable lightweight off screen opengl context
  hunter_add_package(aglet)
  find_package(aglet CONFIG REQUIRED)
  list(APPEND OGLES_GPGPU_TEST_LIBS aglet::aglet)

  # Use gauze for cross platform ctesting
  hunter_add_package(gauze)
  find_package(gauze CONFIG REQUIRED)
  list(APPEND OGLES_GPGPU_TEST_LIBS gauze::gauze)

  hunter_add_package(OpenCV)
  find_package(OpenCV REQUIRED)
  list(APPEND OGLES_GPGPU_TEST_LIBS "${OpenCV_LIBS}")
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
