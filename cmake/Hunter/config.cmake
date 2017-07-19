set(OPENCV_CMAKE_ARGS

  BUILD_opencv_core=ON
  BUILD_opencv_imgproc=ON
  
  BUILD_opencv_imgcodecs=OFF
  BUILD_opencv_video=OFF
  BUILD_opencv_videoio=OFF
  BUILD_opencv_highgui=OFF  
  
  BUILD_opencv_calib3d=OFF
  BUILD_opencv_contrib=OFF
  BUILD_opencv_cudaarithm=OFF
  BUILD_opencv_cudabgsegm=OFF
  BUILD_opencv_cudacodec=OFF
  BUILD_opencv_cudafeatures2d=OFF
  BUILD_opencv_cudafilters=OFF
  BUILD_opencv_cudaimgproc=OFF
  BUILD_opencv_cudalegacy=OFF
  BUILD_opencv_cudaobjdetect=OFF
  BUILD_opencv_cudaoptflow=OFF
  BUILD_opencv_cudastereo=OFF
  BUILD_opencv_cudawarping=OFF
  BUILD_opencv_cudev=OFF
  BUILD_opencv_features2d=OFF
  BUILD_opencv_flann=OFF
  BUILD_opencv_gpu=OFF
  BUILD_opencv_java=OFF
  BUILD_opencv_legacy=OFF
  BUILD_opencv_ml=OFF
  BUILD_opencv_nonfree=OFF
  BUILD_opencv_objdetect=OFF
  BUILD_opencv_ocl=OFF
  BUILD_opencv_photo=OFF
  BUILD_opencv_python=OFF
  BUILD_opencv_shape=OFF
  BUILD_opencv_stitching=OFF
  BUILD_opencv_superres=OFF
  BUILD_opencv_ts=OFF
  BUILD_opencv_videostab=OFF
  BUILD_opencv_viz=OFF
  BUILD_opencv_world=OFF
  BUILD_opencv_apps=OFF  
  
  BUILD_DOCS=OFF
  BUILD_TESTS=OFF
  BUILD_PERF_TESTS=OFF
  BUILD_EXAMPLES=OFF
  ENABLE_NEON=OFF
  BUILD_ANDROID_SERVICE=OFF
  BUILD_ANDROID_EXAMPLES=OFF
  BUILD_ZLIB=OFF ## HUNTER
  BUILD_TIFF=OFF ## HUNTER
  BUILD_PNG=OFF  ## HUNTER
  BUILD_JPEG=OFF ## HUNTER
  ANDROID_EXAMPLES_WITH_LIBS=OFF    # "Build binaries of Android examples with native libraries"

  ### Custom ARGS ###
  WITH_PNG=OFF             # "Include PNG support"
  WITH_TIFF=OFF           # "Include TIFF support"
  WITH_JASPER=OFF          # "Include JPEG2K support"
  WITH_JPEG=OFF            # "Include JPEG support"

  WITH_OPENCL=NO
  HAVE_OPENCL=NO
  HAVE_EIGEN=NO
  HAVE_CUFFT=NO
  HAVE_CUBLAS=NO
  HAVE_CUDA=NO

  WITH_QTKIT=NO

  WITH_PTHREADS_PF=OFF    # "Use pthreads-based parallel_for"
  WITH_TBB=OFF            # "Include Intel TBB support"
  WITH_1394=OFF           # "Include IEEE1394 support"
  WITH_AVFOUNDATION=OFF   # "Use AVFoundation for Video I/O"
  WITH_CARBON=OFF         # "Use Carbon for UI instead of Cocoa"
  WITH_VTK=OFF            # "Include VTK library support (and build opencv_viz module eiher)"
  WITH_CUDA=OFF           # "Include NVidia Cuda Runtime support"
  WITH_CUFFT=OFF          # "Include NVidia Cuda Fast Fourier Transform (FFT) library support"
  WITH_CUBLAS=OFF         # "Include NVidia Cuda Basic Linear Algebra Subprograms (BLAS) library support"
  WITH_NVCUVID=OFF        # "Include NVidia Video Decoding library support"
  WITH_EIGEN=OFF          # "Include Eigen2/Eigen3 support"
  WITH_VFW=OFF            # "Include Video for Windows support"
  WITH_FFMPEG=OFF         # "Include FFMPEG support"
  WITH_GSTREAMER=OFF      # "Include Gstreamer support"
  WITH_GSTREAMER_0_10=OFF # "Enable Gstreamer 0.10 support (instead of 1.x)"
  WITH_GTK=OFF            # "Include GTK support"
  WITH_GTK_2_X=OFF        # "Use GTK version 2"
  WITH_IPP=OFF            # "Include Intel IPP support"
  WITH_WEBP=OFF           # "Include WebP support"
  WITH_OPENEXR=OFF        # "Include ILM support via OpenEXR"
  WITH_OPENGL=OFF         # "Include OpenGL support"
  WITH_OPENNI=OFF         # "Include OpenNI support"
  WITH_OPENNI2=OFF        # "Include OpenNI2 support"
  WITH_PVAPI=OFF          # "Include Prosilica GigE support"
  WITH_GIGEAPI=OFF        # "Include Smartek GigE support"
  WITH_QT=OFF             # "Build with Qt Backend support"
  WITH_WIN32UI=OFF        # "Build with Win32 UI Backend support"
  WITH_QUICKTIME=OFF      # "Use QuickTime for Video I/O insted of QTKit"
  WITH_OPENMP=OFF         # "Include OpenMP support"
  WITH_CSTRIPES=OFF       # "Include C= support"
  WITH_UNICAP=OFF         # "Include Unicap support (GPL)"
  WITH_V4L=OFF            # "Include Video 4 Linux support"
  WITH_LIBV4L=OFF         # "Use libv4l for Video 4 Linux support"
  WITH_DSHOW=OFF          # "Build VideoIO with DirectShow support"
  WITH_MSMF=OFF           # "Build VideoIO with Media Foundation support"
  WITH_XIMEA=OFF          # "Include XIMEA cameras support"
  WITH_XINE=OFF           # "Include Xine support (GPL)"
  WITH_CLP=OFF            # "Include Clp support (EPL)"
  WITH_OPENCL=OFF         # "Include OpenCL Runtime support"
  WITH_OPENCL_SVM=OFF     # "Include OpenCL Shared Virtual Memory support"
  WITH_OPENCLAMDFFT=OFF   # "Include AMD OpenCL FFT library support"
  WITH_OPENCLAMDBLAS=OFF  # "Include AMD OpenCL BLAS library support"
  WITH_DIRECTX=OFF        # "Include DirectX support"
  WITH_INTELPERC=OFF      # "Include Intel Perceptual Computing support"
  WITH_IPP_A=OFF          # "Include Intel IPP_A support"
  WITH_GDAL=OFF           # "Include GDAL Support"
  WITH_GPHOTO2=OFF        # "Include gPhoto2 library support"
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

hunter_config(aglet GIT_SUBMODULE "3rdparty/aglet" CMAKE_ARGS AGLET_OPENGL_ES3=${aglet_es3})

