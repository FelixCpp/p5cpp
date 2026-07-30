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
  ./src/p5cpp/application/engine.cpp
  ./src/p5cpp/application/frame_api.cpp
  ./src/p5cpp/application/frame_component.cpp
  ./src/p5cpp/application/frame_module.cpp
  ./src/p5cpp/application/input_api.cpp
  ./src/p5cpp/application/input_component.cpp
  ./src/p5cpp/application/input_module.cpp
  ./src/p5cpp/application/logging.cpp
  ./src/p5cpp/application/module.cpp
  ./src/p5cpp/application/sketch_module.cpp
  ./src/p5cpp/application/window.cpp
  ./src/p5cpp/application/window_api.cpp
  ./src/p5cpp/application/window_module.cpp

  ./src/p5cpp/audio/audio_api.cpp
  ./src/p5cpp/audio/audio_component.cpp
  ./src/p5cpp/audio/audio_module.cpp
  ./src/p5cpp/audio/audio_stream.cpp
  ./src/p5cpp/audio/sound.cpp

  ./src/p5cpp/graphics/antialiased_canvas.cpp
  ./src/p5cpp/graphics/color.cpp
  ./src/p5cpp/graphics/font.cpp
  ./src/p5cpp/graphics/framebuffer.cpp
  ./src/p5cpp/graphics/fullscreen_pass.cpp
  ./src/p5cpp/graphics/graphics_api.cpp
  ./src/p5cpp/graphics/graphics_component.cpp
  ./src/p5cpp/graphics/graphics_module.cpp
  ./src/p5cpp/graphics/internal_shaders.cpp
  ./src/p5cpp/graphics/matrix_stack.cpp
  ./src/p5cpp/graphics/pixels.cpp
  ./src/p5cpp/graphics/render_group_recorder.cpp
  ./src/p5cpp/graphics/render_state.cpp
  ./src/p5cpp/graphics/render_state_stack.cpp
  ./src/p5cpp/graphics/renderer.cpp
  ./src/p5cpp/graphics/shader.cpp
  ./src/p5cpp/graphics/shaping.cpp
  ./src/p5cpp/graphics/stb_image_impl.cpp
  ./src/p5cpp/graphics/stroker.cpp
  ./src/p5cpp/graphics/tess.cpp
  ./src/p5cpp/graphics/texture.cpp
  ./src/p5cpp/graphics/uniform_cache.cpp

  ./src/p5cpp/math/matrix4x4.cpp
  ./src/p5cpp/math/noise.cpp
  ./src/p5cpp/math/random.cpp

  ./src/p5cpp/system/timing.cpp
  ./src/p5cpp/system/utf8_view.cpp

  ./src/p5cpp/p5cpp.cpp
)

target_link_libraries(p5cpp PRIVATE glfw glad freetype libtess2 miniaudio harfbuzz stb)
target_include_directories(p5cpp PUBLIC include)
target_include_directories(p5cpp PRIVATE src)
add_subdirectory(examples)
