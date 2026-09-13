#pragma once

#include <string_view>

namespace p5
{
    namespace detail
    {
        inline static constexpr std::string_view defaultVertexShaderSource = R"(
            struct VertexOutput {
                @builtin(position) position: vec4f,
                @location(0) texCoord: vec2f,
                @location(1) color: vec4f,
            };

            @group(0) @binding(0) var<uniform> u_ProjectionMatrix: mat4x4f;

            @vertex
            fn vs_main(
                @location(0) a_Position: vec2f,
                @location(1) a_TexCoord: vec2f,
                @location(2) a_Color: vec4f,
            ) -> VertexOutput {
                var out: VertexOutput;
                out.position = u_ProjectionMatrix * vec4f(a_Position, 0.0, 1.0);
                out.texCoord = a_TexCoord;
                out.color = a_Color;
                return out;
            }
        )";
    } // namespace detail
} // namespace p5
