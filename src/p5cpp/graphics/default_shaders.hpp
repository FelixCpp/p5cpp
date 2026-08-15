#pragma once

#include <string_view>

namespace p5
{
    namespace detail
    {
        // The vertex stage every built-in shader (fill, text, and effect shaders built via the
        // single-argument loadShaderFromMemory()) shares: plain position/UV/color pass-through plus the
        // 2D orthographic projection. Shared between graphics.cpp (default fill/text shaders) and
        // shader.cpp (effect-shader wrapping) so there is one canonical vertex contract.
        inline static constexpr std::string_view defaultVertexShaderSource = R"(
            #version 410

            layout (location = 0) in vec2 a_Position;
            layout (location = 1) in vec2 a_TexCoord;
            layout (location = 2) in vec4 a_Color;

            uniform mat4 u_ProjectionMatrix;

            out vec2 v_TexCoord;
            out vec4 v_Color;

            void main()
            {
                gl_Position = u_ProjectionMatrix * vec4(a_Position, 0.0, 1.0);
                v_TexCoord = a_TexCoord;
                v_Color = a_Color;
            }
        )";
    } // namespace detail
} // namespace p5
