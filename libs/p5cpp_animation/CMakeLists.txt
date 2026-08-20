add_library(
  p5cpp_animation STATIC
  ./src/p5cpp_animation.cpp
)

target_include_directories(p5cpp_animation PUBLIC include)
target_link_libraries(p5cpp_animation PUBLIC p5cpp)

if (MSVC)
  target_compile_options(p5cpp_animation PRIVATE /W4)
else()
  target_compile_options(p5cpp_animation PRIVATE -Wall -Wextra)
endif()
