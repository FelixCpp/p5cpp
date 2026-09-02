cmake_minimum_required(VERSION 3.20)
project(p5cpp)

add_subdirectory(external)
add_subdirectory(libs/p5cpp)
add_subdirectory(libs/p5cpp_animation)
add_subdirectory(libs/p5cpp_audio)
add_subdirectory(libs/p5cpp_gif)

option(P5CPP_BUILD_EXAMPLES "Build the p5cpp example sketches" ON)
if (P5CPP_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
