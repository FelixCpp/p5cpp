# glfw3webgpu (https://github.com/eliemichel/glfw3webgpu, MIT) -- vendored as source (not a git
# submodule) since it's two small files without an upstream release process to pin against.
# Creates a WGPUSurface from a GLFWwindow* per-platform (Cocoa/X11/Wayland/Win32), so our own
# code doesn't hand-roll the NSWindow -> CAMetalLayer -> WGPUSurfaceDescriptor glue (and the
# equivalents on other platforms, should this project ever build there).
#
# glfw3webgpu.c uses Objective-C message syntax directly in a .c file on Apple platforms, so it
# must be compiled as Objective-C there (see CameraCapture's -fobjc-arc for the same pattern).

add_library(glfw3webgpu STATIC glfw3webgpu.c)
target_include_directories(glfw3webgpu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(glfw3webgpu PUBLIC glfw wgpu-native)

if (APPLE)
    # glfw3webgpu.c picks its platform branch off _GLFW_COCOA/_GLFW_X11/etc, which are internal
    # macros GLFW only defines while compiling *itself* -- not visible to external consumers of
    # its public headers. Define GLFW_EXPOSE_NATIVE_COCOA directly instead of relying on that
    # detection.
    set_source_files_properties(glfw3webgpu.c PROPERTIES
        COMPILE_FLAGS "-x objective-c"
        COMPILE_DEFINITIONS "GLFW_EXPOSE_NATIVE_COCOA"
    )
    target_link_libraries(glfw3webgpu PRIVATE "-framework Cocoa")
endif()
