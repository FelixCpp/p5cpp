cmake_minimum_required(VERSION 3.20)
project(p5cpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external/glfw)
add_subdirectory(external/freetype)
add_subdirectory(external/glad)
add_subdirectory(external/miniaudio)

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
  p5cpp STATIC
  ./src/modules/input/input_module.cpp
  ./src/modules/frame/frame_module.cpp
  ./src/modules/window/window_module.cpp
  ./src/modules/sketch/sketch_module.cpp
  ./src/modules/rendering/rendering_module.cpp
  ./src/engine.cpp
  ./src/module.cpp
  ./src/services/window.cpp
  ./src/services/renderer.cpp
  ./src/services/uniform_cache.cpp
  ./src/color.cpp
  ./src/font.cpp
  ./src/framebuffer.cpp
  ./src/linepath.cpp
  ./src/math.cpp
  ./src/matrix_stack.cpp
  ./src/noise.cpp
  ./src/p5cpp.cpp
  ./src/render_state.cpp
  ./src/render_state_stack.cpp
  ./src/shader.cpp
  ./src/stroker.cpp
  ./src/tess.cpp
  ./src/texture.cpp
  ./src/timing.cpp
  ./src/utf8_view.cpp
)

target_link_libraries(p5cpp PRIVATE glfw glad freetype libtess2 miniaudio)
target_include_directories(p5cpp PUBLIC include)
add_subdirectory(examples)
