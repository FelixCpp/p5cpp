#pragma once

#include <p5cpp/p5cpp.hpp>

#include <string>
#include <unordered_map>

namespace p5
{
    struct ShaderImpl
    {
        uint32_t programId = 0;
        std::unordered_map<std::string, int32_t> uniformLocationCache;
        std::unordered_map<std::string, UniformValue> uniforms;

        ShaderImpl() = default;
        ShaderImpl(const ShaderImpl&) = delete;
        ShaderImpl& operator=(const ShaderImpl&) = delete;
        ~ShaderImpl();
    };
} // namespace p5
