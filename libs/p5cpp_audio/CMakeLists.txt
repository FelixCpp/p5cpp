add_library(p5cpp_audio STATIC
  ./src/p5cpp_audio/audio_engine.cpp
  ./src/p5cpp_audio/audio_plugin.cpp
  ./src/p5cpp_audio/p5cpp_audio.cpp
  ./src/p5cpp_audio/sound.cpp
  ./src/p5cpp_audio/sound_resource.cpp
)

target_include_directories(p5cpp_audio PUBLIC include)
target_include_directories(p5cpp_audio PRIVATE src)

target_link_libraries(p5cpp_audio PRIVATE p5cpp miniaudio)

if (MSVC)
  target_compile_options(p5cpp_audio PRIVATE /W4)
else()
  target_compile_options(p5cpp_audio PRIVATE -Wall -Wextra)
endif()
