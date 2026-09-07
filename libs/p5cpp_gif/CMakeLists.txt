add_library(p5cpp_gif STATIC
  ./src/p5cpp_gif/gif_writer.cpp
  ./src/p5cpp_gif/gif_frame_timer.cpp
  ./src/p5cpp_gif/gif_recorder.cpp
  ./src/p5cpp_gif/gif_recording_resource.cpp
  ./src/p5cpp_gif/p5cpp_gif.cpp
)

target_include_directories(p5cpp_gif PUBLIC include)
target_include_directories(p5cpp_gif PRIVATE src)
target_link_libraries(p5cpp_gif PRIVATE p5cpp msf_gif)
target_compile_features(p5cpp_gif PUBLIC cxx_std_23)

if (MSVC)
  target_compile_options(p5cpp_gif PRIVATE /W4)
else()
  target_compile_options(p5cpp_gif PRIVATE -Wall -Wextra)
endif()
