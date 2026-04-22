cmake_minimum_required(VERSION 3.20)
project(p5)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external/glfw)
add_subdirectory(external/glad)

add_executable(app src/main.cpp src/p5.cpp src/p5_internal.cpp src/geometry.cpp)

target_link_libraries(app PRIVATE glfw glad)
target_include_directories(app PRIVATE ./external/libtess3/Source/)
