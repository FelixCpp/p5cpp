#pragma once

#include <p5cpp/p5cpp.hpp>

#include <webgpu/webgpu.h>

#include <string>
#include <unordered_map>

namespace p5
{
    struct ShaderUniformSlot
    {
        uint32_t byteOffset = 0;
        uint32_t byteSize = 0;
    };

    struct ShaderImpl
    {
        WGPUShaderModule vertexModule = nullptr;
        WGPUShaderModule fragmentModule = nullptr;

        std::unordered_map<std::string, ShaderUniformSlot> extraUniformSlots;
        uint32_t extraUniformsByteSize = 0;

        ShaderImpl() = default;
        ShaderImpl(const ShaderImpl&) = delete;
        ShaderImpl& operator=(const ShaderImpl&) = delete;
        ~ShaderImpl();
    };
} // namespace p5
