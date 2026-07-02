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

    std::unique_ptr<Shader> createPrimitiveShader()
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

    std::unique_ptr<Shader> createTextShader()
    {
        return loadShader(textVSource, textFSource);
    }
} // namespace p5cpp
