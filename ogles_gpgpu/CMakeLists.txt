include(sugar_include)
sugar_include(.)

add_library(ogles_gpgpu ${OGLES_GPGPU_SRCS})

if(OGLES_GPGPU_VERBOSE)
  target_compile_definitions(ogles_gpgpu PUBLIC OGLES_GPGPU_VERBOSE=1)
endif()

## #################################################################
## Dependencies - OpenGL stuff
## #################################################################

if(APPLE)
  if(IOS)
    target_link_libraries(ogles_gpgpu PUBLIC
      "-framework ImageIO"
      "-framework CoreFoundation"
      "-framework CoreVideo"
      "-framework Foundation" # NSLog
      "-framework OpenGLES"      
      )    
  else()
    target_link_libraries(ogles_gpgpu PUBLIC
      "-framework ImageIO"
      "-framework CoreFoundation"
      "-framework CoreVideo"
      "-framework OpenGL"
      )
  endif()
elseif(ANDROID)
  find_package(Android REQUIRED)
  find_package(Log REQUIRED)
  find_package(EGL REQUIRED)

  if(OGLES_GPGPU_OPENGL_ES3)
    find_package(OpenGLES3 REQUIRED)
    set(ogles_gpgpu_opengl_lib GLESv3)
  else()
    find_package(OpenGLES2 REQUIRED)
    set(ogles_gpgpu_opengl_lib GLESv2)
  endif()
  target_link_libraries(ogles_gpgpu PUBLIC log android EGL ${ogles_gpgpu_opengl_lib})

  
  target_compile_definitions(ogles_gpgpu PUBLIC EGL_EGLEXT_PROTOTYPES GL_GLEXT_PROTOTYPES)
else()

  if(OGLES_GPGPU_USE_OSMESA)

    # Note: Currently OSMesa is used for CPU OpenGL shader processing,
    # primarily to supporting testing on HW platforms that lack GPU
    # resources.  This is currently tested on Linux, although this could
    # in theory be tested on other platforms.  There are a number of
    # platform specific optimizations for iOS and Android that would
    # have to be made option in order to support this.  For now the
    # OSMESA option is excluded from those platforms in the CMake control
    # logic.
    
    find_library(OSMESA_LIBRARY OSMesa)
    if(NOT OSMESA_LIBRARY)
      message(FATAL_ERROR "OGLES_GPGPU was configured to use OSMesa, but the library could not be found")
    endif()

    get_filename_component(OSMESA_PATH ${OSMESA_LIBRARY} DIRECTORY)

    find_library(GLAPI_LIBRARY glapi HINTS ${OSMESA_PATH} PATHS ${OSMESA_PATH})
    if(NOT GLAPI_LIBRARY)
      message(FATAL_ERROR "OGLES_GPGPU was configured to use OSMesa, but the GLAPI dependency was not found")
    endif()

    find_library(EGL_LIBRARY EGL HINTS ${OSMESA_PATH} PATHS ${OSMESA_PATH})
    if(NOT EGL_LIBRARY)
      message(FATAL_ERROR "EGL_LIBRARY was configured to use OSMesa, but the EGL dependency was not found")
    endif()
    mark_as_advanced(OSMESA_LIBRARY GLAPI_LIBRARY EGL_LIBRARY)

    # Create a new target specfiically for OSMesa (cpu shader) based testing 
    # (We will never create ogles_gpgpu libraries w/ CPU OpenGL processing.)
    add_library(ogles_gpgpu_cpu ${OGLES_GPGPU_SRCS})

    if(OGLES_GPGPU_VERBOSE)
      target_compile_definitions(ogles_gpgpu_cpu PUBLIC OGLES_GPGPU_VERBOSE=1)
    endif()    

    target_link_libraries(ogles_gpgpu_cpu PUBLIC ${OSMESA_LIBRARY} ${GLAPI_LIBRARY} ${EGL_LIBRARY})

    set_property(TARGET ogles_gpgpu_cpu PROPERTY FOLDER "libs/ogles_gpgpu")
    target_include_directories(
      ogles_gpgpu_cpu PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>"
      )

    if(OGLES_GPGPU_OPENGL_ES3)    
      target_compile_definitions(ogles_gpgpu_cpu PUBLIC OGLES_GPGPU_OPENGL_ES3=1)
    endif()
    
  endif()

  # Link epxported ogles_gpgpu with the stanrda OpenGL lib
  find_package(OpenGL REQUIRED)
  target_link_libraries(ogles_gpgpu PUBLIC OpenGL::GL)
  
  if(MSVC)
    # ogles_gpgpu/platform/opengl/gl_includes.h: #include <gl/glew.h>
    hunter_add_package(glew)
    find_package(glew CONFIG REQUIRED)
    target_link_libraries(ogles_gpgpu PUBLIC glew::glew)
    target_compile_definitions(ogles_gpgpu PUBLIC NOMINMAX) # avoid std::{min,max} conflicts
    target_compile_definitions(ogles_gpgpu PUBLIC _USE_MATH_DEFINES) # M_PI, etc
  endif()
endif()
  
set_property(TARGET ogles_gpgpu PROPERTY FOLDER "libs/ogles_gpgpu")
target_include_directories(
    ogles_gpgpu PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>"
    )

if(OGLES_GPGPU_OPENGL_ES3)    
  target_compile_definitions(ogles_gpgpu PUBLIC OGLES_GPGPU_OPENGL_ES3=1)
endif()

## #################################################################
## Testing: 
## #################################################################

if(OGLES_GPGPU_BUILD_TESTS)
  add_subdirectory(ut)
endif()

#
# Provide interface lib for clean package config use
#

if(NOT OGLES_GPGPU_INSTALL)
  return()
endif()

### Install ###
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")

set(config_install_dir "lib/cmake/${PROJECT_NAME}")
set(include_install_dir "include")

set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(targets_export_name "${PROJECT_NAME}Targets")
set(namespace "${PROJECT_NAME}::")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${version_config}" COMPATIBILITY SameMajorVersion
)

# Note: use 'targets_export_name'
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/Config.cmake.in"
    "${project_config}"
     INSTALL_DESTINATION "${config_install_dir}"
)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT "${targets_export_name}"
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin"
    INCLUDES DESTINATION "${include_install_dir}"
)

install(
    FILES
    ${OGLES_GPGPU_PUBLIC_HDRS}
    DESTINATION "${include_install_dir}/${PROJECT_NAME}"
)

install(
  DIRECTORY "${CMAKE_SOURCE_DIR}/ogles_gpgpu/" # Trailing slash is required
  DESTINATION "${include_install_dir}/${PROJECT_NAME}"
  FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
  )

install(
  FILES "${project_config}" "${version_config}"
  DESTINATION "${config_install_dir}"
  )

install(
  EXPORT "${targets_export_name}"
  NAMESPACE "${namespace}"
  DESTINATION "${config_install_dir}"
  )
