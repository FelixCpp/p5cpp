add_library(p5cpp_webcam STATIC
  ./src/p5cpp_webcam/capture_resource.cpp
  ./src/p5cpp_webcam/p5cpp_webcam.cpp
  ./src/p5cpp_webcam/gpu_pixel_stream.cpp
)

target_include_directories(p5cpp_webcam PUBLIC include)
target_include_directories(p5cpp_webcam PRIVATE src)
target_link_libraries(p5cpp_webcam PRIVATE p5cpp ccap)
target_compile_features(p5cpp_webcam PUBLIC cxx_std_23)

set_target_properties(p5cpp_webcam PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

if (MSVC)
  target_compile_options(p5cpp_webcam PRIVATE /W4)
else()
  target_compile_options(p5cpp_webcam PRIVATE -Wall -Wextra)
endif()
