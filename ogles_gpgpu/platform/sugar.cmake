# This file generated automatically by:
#   generate_sugar_files.py
# see wiki for more info:
#   https://github.com/ruslo/sugar/wiki/Collecting-sources

if(DEFINED OGLES_GPGPU_PLATFORM_SUGAR_CMAKE_)
  return()
else()
  set(OGLES_GPGPU_PLATFORM_SUGAR_CMAKE_ 1)
endif()

include(sugar_include)

# We drop support for sugar_include(android) due to platforms
# specific extensions that require dlopen() based system lib
# access, since dlopen() calls are blocked in recent versions
# of Android.
if(IOS)
  sugar_include(ios)
else()
  sugar_include(opengl)
endif()
