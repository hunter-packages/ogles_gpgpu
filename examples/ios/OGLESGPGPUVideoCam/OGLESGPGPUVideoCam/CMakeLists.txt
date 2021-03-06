# Copyright (c) 2013-2014, Ruslan Baratov, David Hirvonen
# All rights reserved.

cmake_minimum_required(VERSION 3.0.0)

if(NOT IOS)
  message(FATAL_ERROR "Please use iOS toolchain")
endif()

include_directories("${SUGAR_ROOT}/examples/third_party")
include_directories("${CMAKE_SOURCE_DIR}/src/app/ios")

#sugar_include("./")
sugar_include("${SUGAR_ROOT}/examples/resources/ios/icons")
sugar_include("${SUGAR_ROOT}/examples/resources/ios/images")

set(OGLESGPGPU_VIDEOCAM_SRCS
  AppDelegate.h
  AppDelegate.mm
  CamView.h
  CamView.mm
  RootViewController.h
  RootViewController.mm
  main.m
)

set(OGLESGPGPU_VIDEOCAM_RESOURCES ${IOS_ICONS} ${DEFAULT_IOS_IMAGES})

if(NOT XCODE_VERSION)
  sugar_fatal_error("Xcode only")
endif()

add_executable(OGLESGPGPUVideoCam ${OGLESGPGPU_VIDEOCAM_SRCS} ${OGLESGPGPU_VIDEOCAM_RESOURCES})

set_target_properties(
  OGLESGPGPUVideoCam
  PROPERTIES
  MACOSX_BUNDLE YES
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/storyboard.plist.in"
  XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # Universal (iPad + iPhone)
  # http://stackoverflow.com/a/20982506/2288008
  XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
  XCODE_ATTRIBUTE_INSTALL_PATH "${CMAKE_BINARY_DIR}/ProductRelease"
  # By default this setting is empty =>
  # archives not showing up in Xcode organizer.
  # http://stackoverflow.com/a/8102602/2288008
  XCODE_ATTRIBUTE_COMBINE_HIDPI_IMAGES "NO" # If this setting is "YES"
  # application can't pass archive validation.
  # http://stackoverflow.com/a/24040412/2288008
  RESOURCE "${RESOURCES}"
)

set_target_properties(
  OGLESGPGPUVideoCam
  PROPERTIES
  XCODE_ATTRIBUTE_PRODUCT_NAME
  "OGLESGPGPUVideoCam"
  XCODE_ATTRIBUTE_BUNDLE_IDENTIFIER
  "com.elucideye.OGLESGPGPUVideoCam.TestOpencv"
)

set_target_properties(
  OGLESGPGPUVideoCam
  PROPERTIES
  XCODE_ATTRIBUTE_PRODUCT_NAME[variant=Debug]
  "OGLESGPGPUVideoCam-Dbg"
  XCODE_ATTRIBUTE_BUNDLE_IDENTIFIER[variant=Debug]
  "com.elucideye.OGLESGPGPUVideoCam.debug"
  )

find_package(coregraphics REQUIRED)
find_package(foundation REQUIRED)
find_package(uikit REQUIRED)
find_package(quartzcore REQUIRED)
find_package(imageio REQUIRED)
find_package(coregraphics REQUIRED)
find_package(corevideo REQUIRED)
find_package(coremedia REQUIRED)
find_package(corelocation REQUIRED)
find_package(glkit REQUIRED)
find_package(accelerate REQUIRED)
find_package(opengles REQUIRED)
find_package(uikit REQUIRED)

target_link_libraries(
  OGLESGPGPUVideoCam
  ogles_gpgpu
  coregraphics::coregraphics
  foundation::foundation
  uikit::uikit
  quartzcore::quartzcore
  imageio::imageio
  coregraphics::coregraphics
  corevideo::corevideo
  coremedia::coremedia
  corelocation::corelocation
  glkit::glkit
  accelerate::accelerate
  opengles::opengles
  uikit::uikit
  )
