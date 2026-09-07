cmake_minimum_required(VERSION 3.20)
project(p5cpp)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -g")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external)
add_subdirectory(libs/p5cpp)
add_subdirectory(libs/p5cpp_animation)
add_subdirectory(libs/p5cpp_audio)
add_subdirectory(libs/p5cpp_gif)

option(P5CPP_BUILD_EXAMPLES "Build the p5cpp example sketches" ON)
if (P5CPP_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
