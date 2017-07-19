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

if(ANDROID OR IOS)
  set(ogles_gpgpu_is_mobile TRUE)
else()
  set(ogles_gpgpu_is_mobile FALSE)
endif()

# We will use the vanilla OpenGL implementation in case
# of standard OpenGL platforms (i.e., not OpenGL ES) or
# in cases where >= OpenGL ES 3.0 is available.  On those
# platforms we can use PBO for efficient GPU->CPU reads.
# If we are using OpenGL ES 2.0 on mobile devices then
# we will use platform specific extensions to facilitate
# efficient DMA reads of OpenGL textures.
if(OGLES_GPGPU_OPENGL_ES3 OR NOT ${ogles_gpgpu_is_mobile})
  sugar_include(opengl)
else()
  if(IOS)
    sugar_include(ios)
  elseif(ANDROID)
    sugar_include(android )
  endif()
endif()



