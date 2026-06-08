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

add_library(
  p5 STATIC
  ./src/canvas_stack.cpp
  ./src/color.cpp
  ./src/draw_buffer.cpp
  ./src/draw_command.cpp
  ./src/draw_command_list.cpp
  ./src/draw_scope.cpp
  ./src/font.cpp
  ./src/framebuffer.cpp
  ./src/linepath.cpp
  ./src/math.cpp
  ./src/matrix_stack.cpp
  ./src/noise.cpp
  ./src/p5.cpp
  ./src/render_state.cpp
  ./src/render_state_stack.cpp
  ./src/renderer.cpp
  ./src/shader.cpp
  ./src/stroker.cpp
  ./src/tess.cpp
  ./src/texture.cpp
  ./src/timing.cpp
  ./src/uniform_cache.cpp
  ./src/window.cpp)

target_link_libraries(p5 PRIVATE glfw glad freetype libtess2)
target_include_directories(p5 PUBLIC include)
add_subdirectory(examples)
