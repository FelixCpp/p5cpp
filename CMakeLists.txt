cmake_minimum_required(VERSION 3.20)
project(p5)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external/glfw)
add_subdirectory(external/freetype)
add_subdirectory(external/glad)

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

add_executable(
  app
  src/canvas.cpp
  src/drawscope.cpp
  src/font.cpp
  src/framebuffer.cpp
  src/linepath.cpp
  src/main.cpp
  src/math.cpp
  src/matrixstack.cpp
  src/noise.cpp
  src/p5.cpp
  src/rendering.cpp
  src/renderstate.cpp
  src/shader.cpp
  src/stroker.cpp
  src/tess.cpp
  src/texture.cpp
  src/timing.cpp
  src/uniform_cache.cpp
  src/window.cpp)

target_link_libraries(app PRIVATE glfw glad freetype libtess2)

add_executable(
  breakout
  src/canvas.cpp
  src/drawscope.cpp
  src/font.cpp
  src/framebuffer.cpp
  src/linepath.cpp
  src/breakout.cpp
  src/math.cpp
  src/matrixstack.cpp
  src/noise.cpp
  src/p5.cpp
  src/rendering.cpp
  src/renderstate.cpp
  src/shader.cpp
  src/stroker.cpp
  src/tess.cpp
  src/texture.cpp
  src/timing.cpp
  src/uniform_cache.cpp
  src/window.cpp)

target_link_libraries(breakout PRIVATE glfw glad freetype libtess2)

add_executable(
  thetower
  src/canvas.cpp
  src/drawscope.cpp
  src/font.cpp
  src/framebuffer.cpp
  src/linepath.cpp
  src/thetower.cpp
  src/math.cpp
  src/matrixstack.cpp
  src/noise.cpp
  src/p5.cpp
  src/rendering.cpp
  src/renderstate.cpp
  src/shader.cpp
  src/stroker.cpp
  src/tess.cpp
  src/texture.cpp
  src/timing.cpp
  src/uniform_cache.cpp
  src/window.cpp)

target_link_libraries(thetower PRIVATE glfw glad freetype libtess2)

add_executable(
  gravitas
  src/canvas.cpp
  src/drawscope.cpp
  src/font.cpp
  src/framebuffer.cpp
  src/linepath.cpp
  src/gravitas.cpp
  src/math.cpp
  src/matrixstack.cpp
  src/noise.cpp
  src/p5.cpp
  src/rendering.cpp
  src/renderstate.cpp
  src/shader.cpp
  src/stroker.cpp
  src/tess.cpp
  src/texture.cpp
  src/timing.cpp
  src/uniform_cache.cpp
  src/window.cpp)

target_link_libraries(gravitas PRIVATE glfw glad freetype libtess2)
