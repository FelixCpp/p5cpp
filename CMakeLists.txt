cmake_minimum_required(VERSION 3.20)

# std::format's floating-point formatting needs std::to_chars, which Apple's libc++ only
# ships starting with macOS 13.3 -- must be set before project() to actually take effect.
if (APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "13.3" CACHE STRING "Minimum macOS deployment target" FORCE)
endif()

project(p5cpp)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -g")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external)
add_subdirectory(libs/p5cpp)
add_subdirectory(libs/p5cpp_animation)
add_subdirectory(libs/p5cpp_audio)
add_subdirectory(libs/p5cpp_camera)
add_subdirectory(libs/p5cpp_gif)
add_subdirectory(libs/p5cpp_webcam)

option(P5CPP_BUILD_EXAMPLES "Build the p5cpp example sketches" ON)
if (P5CPP_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
