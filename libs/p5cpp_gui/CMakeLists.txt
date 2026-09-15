add_library(p5cpp_gui STATIC
  ./src/p5cpp_gui/p5cpp_gui.cpp
)

target_include_directories(p5cpp_gui PUBLIC include)
target_include_directories(p5cpp_gui PRIVATE src)
target_link_libraries(p5cpp_gui PRIVATE p5cpp)
target_link_libraries(p5cpp_gui PUBLIC microui)
target_compile_features(p5cpp_gui PUBLIC cxx_std_23)

set_target_properties(p5cpp_gui PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

if (MSVC)
  target_compile_options(p5cpp_gui PRIVATE /W4)
else()
  target_compile_options(p5cpp_gui PRIVATE -Wall -Wextra)
endif()
