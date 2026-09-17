#pragma once

#include <cstdint>

namespace p5::wgsl
{
    inline constexpr uint32_t kProjectionBindGroup = 0;
    inline constexpr uint32_t kTextureBindGroup = 1;
    inline constexpr uint32_t kExtraUniformsBindGroup = 2;

    inline constexpr uint32_t kComputeStorageBindGroup = 0;

    inline constexpr const char* kVertexEntryPoint = "vs_main";
    inline constexpr const char* kFragmentEntryPoint = "fs_main";
    inline constexpr const char* kComputeEntryPoint = "main";
} // namespace p5::wgsl
