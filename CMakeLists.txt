cmake_minimum_required(VERSION 3.20)
project(p5)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external/glfw)
add_subdirectory(external/freetype)
add_subdirectory(external/glad)

add_executable(
  app
  src/main.cpp
  src/p5.cpp
  src/renderer.cpp
  src/math.cpp
  src/tess.cpp
  src/stroker.cpp
  src/linepath.cpp
  src/font.cpp
  src/shader.cpp
  src/drawscope.cpp
  src/canvas.cpp
  src/renderpass.cpp)

target_link_libraries(app PRIVATE glfw glad freetype)

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

target_link_libraries(app PRIVATE libtess2)
