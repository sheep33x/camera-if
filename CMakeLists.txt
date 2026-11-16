cmake_minimum_required(VERSION 3.12)
project(CameraArchitectureSample LANGUAGES CXX)
find_package(Threads REQUIRED)
add_library(camera_core
    src/CameraActor.cpp
    src/Coordinator.cpp
    src/Workers.cpp
    src/CameraModel.cpp
)
target_compile_features(camera_core PUBLIC cxx_std_17)
set_target_properties(camera_core PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(camera_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(camera_core PUBLIC Threads::Threads)
add_executable(camera_sample src/main.cpp)
target_link_libraries(camera_sample PRIVATE camera_core)
enable_testing()
add_executable(camera_tests tests/camera_tests.cpp)
target_link_libraries(camera_tests PRIVATE camera_core)
add_test(NAME camera_tests COMMAND camera_tests)
set_tests_properties(camera_tests PROPERTIES TIMEOUT 15)
