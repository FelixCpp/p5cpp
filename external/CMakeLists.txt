# Central place for all third-party dependencies. Submodules under external/*
# ship their own CMakeLists.txt and are simply added as subdirectories. Vendored
# dependencies without CMake support (libtess2, stb) get their targets defined
# here instead, so we never write build files into a git submodule.

add_subdirectory(glfw)
add_subdirectory(freetype)
add_subdirectory(glad)
add_subdirectory(miniaudio)

set(HB_BUILD_SUBSET OFF CACHE BOOL "" FORCE)
set(HB_BUILD_UTILS OFF CACHE BOOL "" FORCE)
set(HB_BUILD_RASTER OFF CACHE BOOL "" FORCE)
set(HB_BUILD_VECTOR OFF CACHE BOOL "" FORCE)
set(HB_BUILD_GPU OFF CACHE BOOL "" FORCE)
add_subdirectory(harfbuzz)

add_library(
  libtess2 STATIC
  libtess2/Source/bucketalloc.c
  libtess2/Source/dict.c
  libtess2/Source/geom.c
  libtess2/Source/mesh.c
  libtess2/Source/priorityq.c
  libtess2/Source/sweep.c
  libtess2/Source/tess.c
)
target_include_directories(libtess2 PUBLIC libtess2/Include)

add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE stb)
target_compile_definitions(stb INTERFACE STB_IMAGE_IMPLEMENTATION STB_IMAGE_WRITE_IMPLEMENTATION STB_PERLIN_IMPLEMENTATION)
