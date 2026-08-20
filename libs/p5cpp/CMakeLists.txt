add_library(
  libtess2 STATIC
  ../../external/libtess2/Source/bucketalloc.c
  ../../external/libtess2/Source/dict.c
  ../../external/libtess2/Source/geom.c
  ../../external/libtess2/Source/mesh.c
  ../../external/libtess2/Source/priorityq.c
  ../../external/libtess2/Source/sweep.c
  ../../external/libtess2/Source/tess.c
)

target_include_directories(libtess2 PUBLIC ../../external/libtess2/Include)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ../../external/stb)
target_compile_definitions(stb INTERFACE STB_IMAGE_IMPLEMENTATION STB_IMAGE_WRITE_IMPLEMENTATION STB_PERLIN_IMPLEMENTATION)

add_library(
  p5cpp STATIC
  ./src/p5cpp/application/kernel.cpp
  ./src/p5cpp/application/lifecycle.cpp
  ./src/p5cpp/application/lifecycle_plugin.cpp
  ./src/p5cpp/application/lifecycle_api.cpp
  ./src/p5cpp/application/plugin.cpp
  ./src/p5cpp/application/next.cpp
  ./src/p5cpp/application/sketch_plugin.cpp
  ./src/p5cpp/window/window.cpp
  ./src/p5cpp/window/window_plugin.cpp
  ./src/p5cpp/window/window_api.cpp
  ./src/p5cpp/input/input.cpp
  ./src/p5cpp/input/input_plugin.cpp
  ./src/p5cpp/input/input_api.cpp
  ./src/p5cpp/graphics/renderer.cpp
  ./src/p5cpp/graphics/tessellators.cpp
  ./src/p5cpp/graphics/framebuffer.cpp
  ./src/p5cpp/graphics/framebuffer_stack.cpp
  ./src/p5cpp/graphics/draw_state_stack.cpp
  ./src/p5cpp/graphics/matrix_stack.cpp
  ./src/p5cpp/graphics/shader.cpp
  ./src/p5cpp/graphics/shape_builder.cpp
  ./src/p5cpp/graphics/graphics.cpp
  ./src/p5cpp/graphics/graphics_api.cpp
  ./src/p5cpp/graphics/graphics_plugin.cpp
  ./src/p5cpp/graphics/texture.cpp
  ./src/p5cpp/graphics/font.cpp
  ./src/p5cpp/random.cpp
  ./src/p5cpp/perlin_noise.cpp
  ./src/p5cpp/p5cpp.cpp
  ./src/p5cpp/log.cpp
)

target_link_libraries(p5cpp PRIVATE glfw glad freetype libtess2 miniaudio harfbuzz stb)
target_include_directories(p5cpp PUBLIC include)
target_include_directories(p5cpp PRIVATE src)

# Scoped to the p5cpp target itself (not set globally) so it doesn't force vendored
# dependencies like harfbuzz to build under C++23; PUBLIC so consumers linking against
# p5cpp (e.g. the examples) inherit the requirement without redeclaring it.
target_compile_features(p5cpp PUBLIC cxx_std_23)

if (MSVC)
  target_compile_options(p5cpp PRIVATE /W4)
else()
  target_compile_options(p5cpp PRIVATE -Wall -Wextra)
endif()
