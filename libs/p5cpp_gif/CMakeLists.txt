add_library(p5cpp_gif STATIC
  ./src/p5cpp_gif.cpp
)

target_include_directories(p5cpp_gif PUBLIC include)
target_link_libraries(p5cpp_gif PRIVATE p5cpp msf_gif)

if (MSVC)
  target_compile_options(p5cpp_gif PRIVATE /W4)
else()
  target_compile_options(p5cpp_gif PRIVATE -Wall -Wextra)
endif()
