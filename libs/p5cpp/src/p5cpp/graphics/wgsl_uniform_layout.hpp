#pragma once

#include <p5cpp/graphics/shader_impl.hpp>

#include <string_view>
#include <unordered_map>
#include <utility>

namespace p5
{
    // Parses the byte layout of a shader's optional @group(2) @binding(0) "extra uniforms"
    // struct, as documented in p5cpp.hpp's loadShaderFromMemory(effectSource) doc comment:
    //
    //   struct SomeName { fieldA: f32, fieldB: vec2f, ... };
    //   @group(2) @binding(0) var<uniform> someVarName: SomeName;
    std::pair<std::unordered_map<std::string, ShaderUniformSlot>, uint32_t> parseExtraUniformLayout(std::string_view fragmentShaderSource);
} // namespace p5
