cmake_minimum_required(VERSION 3.20)
project(p5cpp)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external/glfw)
add_subdirectory(external/freetype)
add_subdirectory(external/glad)
add_subdirectory(external/miniaudio)

set(HB_BUILD_SUBSET OFF CACHE BOOL "" FORCE)
set(HB_BUILD_UTILS OFF CACHE BOOL "" FORCE)
set(HB_BUILD_RASTER OFF CACHE BOOL "" FORCE)
set(HB_BUILD_VECTOR OFF CACHE BOOL "" FORCE)
set(HB_BUILD_GPU OFF CACHE BOOL "" FORCE)
add_subdirectory(external/harfbuzz)

add_subdirectory(libs/p5cpp)
add_subdirectory(libs/p5cpp_animation)

option(P5CPP_BUILD_EXAMPLES "Build the p5cpp example sketches" ON)
if (P5CPP_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
