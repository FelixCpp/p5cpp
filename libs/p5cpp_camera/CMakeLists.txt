add_library(p5cpp_camera STATIC
  ./src/p5cpp_camera/p5cpp_camera.cpp
)

target_include_directories(p5cpp_camera PUBLIC include)
target_include_directories(p5cpp_camera PRIVATE src)
target_link_libraries(p5cpp_camera PRIVATE p5cpp)
target_compile_features(p5cpp_camera PUBLIC cxx_std_23)

set_target_properties(p5cpp_camera PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

if (MSVC)
  target_compile_options(p5cpp_camera PRIVATE /W4)
else()
  target_compile_options(p5cpp_camera PRIVATE -Wall -Wextra)
endif()
