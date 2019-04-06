set(test_name OGLES_GPGPU_Test)
set(test_app test-ogles_gpgpu)

# Use GTest as unit test framework
hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)
list(APPEND OGLES_GPGPU_TEST_LIBS GTest::gtest)

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

add_executable(${test_app} test-ogles_gpgpu.cpp)

if(TARGET ogles_gpgpu_cpu)
  target_link_libraries(${test_app} PUBLIC ogles_gpgpu_cpu ${OGLES_GPGPU_TEST_LIBS})
else()
  target_link_libraries(${test_app} PUBLIC ogles_gpgpu ${OGLES_GPGPU_TEST_LIBS})
endif()
  
set_property(TARGET ${test_app} PROPERTY FOLDER "app/tests")

if(OGLES_GPGPU_DO_GPU_TESTING)
  # TODO: Lightweight portable OpenGL context for mobile platforms  
  gauze_add_test(
    NAME "${test_name}"
    COMMAND "${test_app}"
    )
endif()
