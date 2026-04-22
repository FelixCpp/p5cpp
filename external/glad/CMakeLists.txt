cmake_minimum_required(VERSION 3.10)

add_library(glad STATIC
    src/glad.c
)

target_include_directories(glad PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Optional aber sauber:
target_compile_features(glad PUBLIC c_std_99)
