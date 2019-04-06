set(OPENCV_CMAKE_ARGS
  WITH_JASPER=OFF
  BUILD_JASPER=OFF
  WITH_WEBP=OFF
  BUILD_WEBP=OFF
  WITH_OPENEXR=OFF
  BUILD_OPENEXR=OFF
  BUILD_PROTOBUF=OFF   #  -/-
  BUILD_LIBPROTOBUF_FROM_SOURCES=NO    
  BUILD_LIST=core,imgproc,videoio,highgui
)

# Try to build very small OpenCV to support unit tests
hunter_config(OpenCV VERSION ${HUNTER_OpenCV_VERSION} CMAKE_ARGS "${OPENCV_CMAKE_ARGS}")

if(OGLES_GPGPU_USE_OSMESA)
  hunter_config(glfw VERSION ${HUNTER_glfw_VERSION} CMAKE_ARGS GLFW_USE_OSMESA=ON)
endif()

message("AGLET_OPENGL_ES3=${OGLES_GPGPU_OPENGL_ES3}")

set(aglet_url "https://github.com/elucideye/aglet/archive/v1.3.3.tar.gz")
set(aglet_sha1 432ad86638c30d221ad444ab73af214c2fe5a180)
set(aglet_args
  "AGLET_USE_EGL=${OGLES_GPGPU_USE_EGL}"
  "AGLET_OPENGL_ES2=${OGLES_GPGPU_OPENGL_ES2}"
  "AGLET_OPENGL_ES3=${OGLES_GPGPU_OPENGL_ES3}"
)

#hunter_config(aglet VERSION ${HUNTER_aglet_VERSION} CMAKE_ARGS AGLET_OPENGL_ES3=${aglet_es3})
hunter_config(aglet URL ${aglet_url} SHA1 ${aglet_sha1} CMAKE_ARGS ${aglet_args})

