cmake_minimum_required(VERSION 3.20)
project(p5cpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
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

add_library(
  libtess2 STATIC
  ./external/libtess2/Source/bucketalloc.c
  ./external/libtess2/Source/dict.c
  ./external/libtess2/Source/geom.c
  ./external/libtess2/Source/mesh.c
  ./external/libtess2/Source/priorityq.c
  ./external/libtess2/Source/sweep.c
  ./external/libtess2/Source/tess.c)

target_include_directories(libtess2 PUBLIC ./external/libtess2/Include)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ./external/stb)

add_library(
  p5cpp STATIC
  ./src/p5cpp/application/kernel.cpp
  ./src/p5cpp/application/lifecycle.cpp
  ./src/p5cpp/application/lifecycle_plugin.cpp
  ./src/p5cpp/application/plugin.cpp
  ./src/p5cpp/application/next.cpp
  ./src/p5cpp/application/sketch_plugin.cpp
  ./src/p5cpp/window/window.cpp
  ./src/p5cpp/window/window_plugin.cpp
  ./src/p5cpp/p5cpp.cpp
)

target_link_libraries(p5cpp PRIVATE glfw glad freetype libtess2 miniaudio harfbuzz stb)
target_include_directories(p5cpp PUBLIC include)
target_include_directories(p5cpp PRIVATE src)
add_subdirectory(examples)
