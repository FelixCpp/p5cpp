#include "shader.hpp"

#include <p5cpp.hpp>
#include <string>
#include <unordered_map>

#include <glad/glad.h>

namespace p5cpp
{
    class OpenGLShader : public Shader
    {
    public:
        static std::unique_ptr<Shader> create(std::string_view vertexSource, std::string_view fragmentSource)
        {
            const char* vertexSourceCStr = vertexSource.data();
            const char* fragmentSourceCStr = fragmentSource.data();

            const uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertexShader, 1, &vertexSourceCStr, nullptr);
            glCompileShader(vertexShader);

            {
                GLint success = GL_FALSE;
                glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
                if (success == GL_FALSE) {
                    GLint logLength = 0;
                    glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &logLength);
                    std::string log(logLength, '\0');
                    glGetShaderInfoLog(vertexShader, logLength, nullptr, log.data());
                    error("Failed to compile vertex shader:\n" + log);
                    glDeleteShader(vertexShader);
                    return nullptr;
                }
            }

            const uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragmentShader, 1, &fragmentSourceCStr, nullptr);
            glCompileShader(fragmentShader);

            {
                GLint success = GL_FALSE;
                glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
                if (success == GL_FALSE) {
                    GLint logLength = 0;
                    glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &logLength);
                    std::string log(logLength, '\0');
                    glGetShaderInfoLog(fragmentShader, logLength, nullptr, log.data());
                    error("Failed to compile fragment shader:\n" + log);
                    glDeleteShader(vertexShader);
                    glDeleteShader(fragmentShader);
                    return nullptr;
                }
            }

            const uint32_t programId = glCreateProgram();
            glAttachShader(programId, vertexShader);
            glAttachShader(programId, fragmentShader);
            glLinkProgram(programId);

            {
                GLint success = GL_FALSE;
                glGetProgramiv(programId, GL_LINK_STATUS, &success);
                if (success == GL_FALSE) {
                    GLint logLength = 0;
                    glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);
                    std::string log(logLength, '\0');
                    glGetProgramInfoLog(programId, logLength, nullptr, log.data());
                    error("Failed to link shader program:\n" + log);
                    glDeleteShader(vertexShader);
                    glDeleteShader(fragmentShader);
                    glDeleteProgram(programId);
                    return nullptr;
                }
            }

            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);

            return std::unique_ptr<OpenGLShader>(new OpenGLShader(programId));
        }

        ~OpenGLShader() override
        {
            glDeleteProgram(programId);
        }

        GLint getUniformLocation(std::string_view name) override
        {
            const auto itr = uniformLocationCache.find(std::string(name));
            if (itr != uniformLocationCache.end()) {
                return itr->second;
            }

            const int location = glGetUniformLocation(programId, name.data());
            if (location == -1) {
                warning("Uniform '" + std::string(name) + "' not found in shader");
            }

            uniformLocationCache[std::string(name)] = location;
            return location;
        }

        uint32_t getRendererId() const override
        {
            return programId;
        }

    private:
        explicit OpenGLShader(uint32_t programId)
            : programId(programId)
        {
        }

        uint32_t programId;
        std::unordered_map<std::string, int> uniformLocationCache;
    };
} // namespace p5cpp

namespace p5cpp
{
    std::unique_ptr<Shader> loadShader(std::string_view vertexSource, std::string_view fragmentSource)
    {
        return OpenGLShader::create(vertexSource, fragmentSource);
    }
} // namespace p5cpp

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

    std::unique_ptr<Shader> create_default_shader()
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

    std::unique_ptr<Shader> create_text_shader()
    {
        return loadShader(textVSource, textFSource);
    }
} // namespace p5cpp
