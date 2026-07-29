#include <p5cpp/graphics/internal_shaders.hpp>

namespace p5cpp
{
    inline static constexpr const char* defaultVSource = R"(
        #version 410 core

        layout (location = 0) in vec2 a_Position;
        layout (location = 1) in vec2 a_TexCoord;
        layout (location = 2) in vec4 a_Color;
        layout (location = 3) in float a_TexIndex;

        out vec2 v_TexCoord;
        out vec4 v_Color;
        out float v_TexIndex;

        uniform mat4 u_ProjectionMatrix;

        void main() {
            gl_Position = u_ProjectionMatrix * vec4(a_Position, 0.0, 1.0);

            v_TexCoord = a_TexCoord;
            v_Color = a_Color;
            v_TexIndex = a_TexIndex;
        }
    )";

    inline static constexpr const char* defaultFSource = R"(
        #version 410 core

        layout (location = 0) out vec4 o_Color;

        in vec2 v_TexCoord;
        in vec4 v_Color;
        in float v_TexIndex;

        uniform sampler2D u_Textures[8];

        void main() {
            vec4 texColor = vec4(1.0);
            switch(int(v_TexIndex)) {
                case 0: texColor = texture(u_Textures[0], v_TexCoord); break;
                case 1: texColor = texture(u_Textures[1], v_TexCoord); break;
                case 2: texColor = texture(u_Textures[2], v_TexCoord); break;
                case 3: texColor = texture(u_Textures[3], v_TexCoord); break;
                case 4: texColor = texture(u_Textures[4], v_TexCoord); break;
                case 5: texColor = texture(u_Textures[5], v_TexCoord); break;
                case 6: texColor = texture(u_Textures[6], v_TexCoord); break;
                case 7: texColor = texture(u_Textures[7], v_TexCoord); break;
                default: break;
            }

            o_Color = v_Color * texColor;
        }
    )";

    Shader createPrimitiveShader()
    {
        return loadShader(defaultVSource, defaultFSource);
    }
} // namespace p5cpp

namespace p5cpp
{
    inline static constexpr const char* textVSource = R"(
        #version 410 core

        layout (location = 0) in vec2 a_Position;
        layout (location = 1) in vec2 a_TexCoord;
        layout (location = 2) in vec4 a_Color;
        layout (location = 3) in float a_TexIndex;

        out vec2 v_TexCoord;
        out vec4 v_Color;
        out float v_TexIndex;

        uniform mat4 u_ProjectionMatrix;

        void main() {
            gl_Position = u_ProjectionMatrix * vec4(a_Position, 0.0, 1.0);

            v_TexCoord = a_TexCoord;
            v_Color = a_Color;
            v_TexIndex = a_TexIndex;
        }
    )";

    inline static constexpr const char* textFSource = R"(
        #version 410 core

        layout (location = 0) out vec4 o_Color;

        in vec2 v_TexCoord;
        in vec4 v_Color;
        in float v_TexIndex;

        uniform sampler2D u_Textures[8];

        void main() {
            vec4 texColor = vec4(1.0);
            switch(int(v_TexIndex)) {
                case 0: texColor = texture(u_Textures[0], v_TexCoord); break;
                case 1: texColor = texture(u_Textures[1], v_TexCoord); break;
                case 2: texColor = texture(u_Textures[2], v_TexCoord); break;
                case 3: texColor = texture(u_Textures[3], v_TexCoord); break;
                case 4: texColor = texture(u_Textures[4], v_TexCoord); break;
                case 5: texColor = texture(u_Textures[5], v_TexCoord); break;
                case 6: texColor = texture(u_Textures[6], v_TexCoord); break;
                case 7: texColor = texture(u_Textures[7], v_TexCoord); break;
                default: break;
            }

            float alpha = texColor.r;
            o_Color = vec4(v_Color.rgb, v_Color.a * alpha);
        }
    )";

    Shader createTextShader()
    {
        return loadShader(textVSource, textFSource);
    }
} // namespace p5cpp

namespace p5cpp
{
    // Reuses defaultVSource: effect passes draw a plain fullscreen quad with the same
    // (position, texcoord, color, texIndex) vertex layout as every other draw call.
    inline static constexpr const char* blurFSource = R"(
        #version 410 core

        layout (location = 0) out vec4 o_Color;

        in vec2 v_TexCoord;
        in vec4 v_Color;
        in float v_TexIndex;

        uniform sampler2D u_Textures[8];
        uniform vec2 u_TexelSize;
        uniform vec2 u_Direction;
        uniform float u_Radius;

        vec4 sampleTex(vec2 uv) {
            switch(int(v_TexIndex)) {
                case 0: return texture(u_Textures[0], uv);
                case 1: return texture(u_Textures[1], uv);
                case 2: return texture(u_Textures[2], uv);
                case 3: return texture(u_Textures[3], uv);
                case 4: return texture(u_Textures[4], uv);
                case 5: return texture(u_Textures[5], uv);
                case 6: return texture(u_Textures[6], uv);
                case 7: return texture(u_Textures[7], uv);
            }
            return vec4(1.0);
        }

        void main() {
            float sigma = max(u_Radius, 0.0001);
            float scale = sigma * 0.25 + 1.0;

            vec4 sum = vec4(0.0);
            float weightSum = 0.0;

            for (int i = -4; i <= 4; ++i) {
                float w = exp(-float(i * i) / (2.0 * sigma * sigma));
                vec2 offset = u_Direction * u_TexelSize * float(i) * scale;
                sum += sampleTex(v_TexCoord + offset) * w;
                weightSum += w;
            }

            o_Color = v_Color * (sum / weightSum);
        }
    )";

    Shader createBlurShader()
    {
        return loadShader(defaultVSource, blurFSource);
    }
} // namespace p5cpp

namespace p5cpp
{
    // Built via loadEffectShader() - the same helper user code uses for custom effects -
    // so these built-ins only need to define `effect()` too.
    inline static constexpr const char* grayscaleSource = R"(
        uniform float u_Amount;

        vec4 effect(vec2 uv, vec2 texelSize, sampler2D tex) {
            vec4 c = texture(tex, uv);
            float luma = dot(c.rgb, vec3(0.299, 0.587, 0.114));
            return vec4(mix(c.rgb, vec3(luma), clamp(u_Amount, 0.0, 1.0)), c.a);
        }
    )";

    Shader createGrayscaleShader()
    {
        return loadEffectShader(grayscaleSource);
    }

    inline static constexpr const char* invertSource = R"(
        uniform float u_Amount;

        vec4 effect(vec2 uv, vec2 texelSize, sampler2D tex) {
            vec4 c = texture(tex, uv);
            return vec4(mix(c.rgb, 1.0 - c.rgb, clamp(u_Amount, 0.0, 1.0)), c.a);
        }
    )";

    Shader createInvertShader()
    {
        return loadEffectShader(invertSource);
    }

    inline static constexpr const char* thresholdSource = R"(
        uniform float u_Amount;

        vec4 effect(vec2 uv, vec2 texelSize, sampler2D tex) {
            vec4 c = texture(tex, uv);
            float luma = dot(c.rgb, vec3(0.299, 0.587, 0.114));
            float v = step(u_Amount, luma);
            return vec4(vec3(v), c.a);
        }
    )";

    Shader createThresholdShader()
    {
        return loadEffectShader(thresholdSource);
    }
} // namespace p5cpp

namespace p5cpp
{
    // Public wrappers exposing the built-ins above as ordinary Shaders (see shader.hpp).
    Shader loadGrayscaleShader() { return createGrayscaleShader(); }
    Shader loadInvertShader() { return createInvertShader(); }
    Shader loadThresholdShader() { return createThresholdShader(); }
    Shader loadBlurShader() { return createBlurShader(); }
} // namespace p5cpp
