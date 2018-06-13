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

# Note: Aglet is currently used to provide an OpenGL context for the unit tests
# We need to make sure it is configured appropriately to provide one of:
# * OpenGL ES 3.0 (or OpenGL 3.0)
# * OpenGL ES 2.0
# The context must be allocated approprately and we need to pull in the correct
# set of headers with mixing them.
if(OGLES_GPGPU_OPENGL_ES3)
  set(aglet_es3 ON)
else()
  set(aglet_es3 OFF)
endif()

hunter_config(aglet VERSION ${HUNTER_aglet_VERSION} CMAKE_ARGS AGLET_OPENGL_ES3=${aglet_es3})
