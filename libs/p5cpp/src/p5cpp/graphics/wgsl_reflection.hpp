#pragma once

#include <p5cpp/graphics/shader_impl.hpp>
#include <p5cpp/graphics/compute_shader_impl.hpp>

#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace p5
{
    struct WgslVarDecl
    {
        uint32_t binding;
        std::string storageMode;
        std::string name;
        std::string typeName;
    };

    std::vector<WgslVarDecl> scanWgslVarDecls(std::string_view source, uint32_t group, std::string_view kind);
    std::pair<std::unordered_map<std::string, ShaderUniformSlot>, uint32_t> parseExtraUniformLayout(std::string_view fragmentShaderSource);
    std::unordered_map<std::string, ComputeBindingSlot> parseComputeBindingLayout(std::string_view computeShaderSource);
} // namespace p5
