# Prebuilt wgpu-native (https://github.com/gfx-rs/wgpu-native), pinned to release
# v29.0.1.1. Not a git submodule -- like glad, wgpu-native ships prebuilt release
# binaries + headers rather than buildable-from-source C code, so the .a and headers
# are vendored directly under lib/ and include/.
#
# Currently macOS-aarch64 only (matches this project's macOS-only build/test scope).
# To bump the pin: download wgpu-macos-aarch64-release.zip from the matching GitHub
# release, replace include/webgpu/ and lib/macos-aarch64/libwgpu_native.a wholesale.

add_library(wgpu-native STATIC IMPORTED GLOBAL)

set_target_properties(wgpu-native PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/macos-aarch64/libwgpu_native.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if (APPLE)
    # CoreFoundation/QuartzCore/Metal are what wgpu-native's own examples link;
    # AppKit/IOKit/IOSurface were additionally required to resolve symbols pulled
    # in by the raw_window_metal crate (e.g. NSKeyValueChangeNewKey) -- confirmed
    # by a standalone link+run smoke test against wgpuCreateInstance/wgpuGetVersion.
    target_link_libraries(wgpu-native INTERFACE
        "-framework CoreFoundation"
        "-framework QuartzCore"
        "-framework Metal"
        "-framework AppKit"
        "-framework IOKit"
        "-framework IOSurface"
    )
endif()
