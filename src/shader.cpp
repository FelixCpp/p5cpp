#include "shader.hpp"
#include <string>
#include <unordered_map>
#include <vector>

#include <glad/glad.h>

namespace p5
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
                    error("Failed to compile shader:\n" + log);
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
                    error("Failed to compile shader:\n" + log);
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

        void setUniform(const NamedUniformVariable& variable) override
        {
            uniforms.push_back(variable);
            const GLint location = getUniformLocation(variable.name);
            if (location == -1) {
                return;
            }

            switch (variable.variable.type) {
                case UniformVariable::Type::float1:
                    glUniform1f(location, variable.variable.floatValue);
                    break;
                case UniformVariable::Type::float2:
                    glUniform2f(location, variable.variable.float2Value.x, variable.variable.float2Value.y);
                    break;
                case UniformVariable::Type::float4:
                    glUniform4f(location, variable.variable.float4Value.x, variable.variable.float4Value.y, variable.variable.float4Value.z, variable.variable.float4Value.w);
                    break;
                case UniformVariable::Type::matrix4x4:
                    glUniformMatrix4fv(location, 1, GL_FALSE, variable.variable.matrix4x4Value.m.data());
                    break;
            }
        }

        UniformSet getUniforms() const override
        {
            return UniformSet {
                .variables = uniforms,
            };
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
        std::vector<NamedUniformVariable> uniforms;
        std::unordered_map<std::string, int> uniformLocationCache;
    };
} // namespace p5

namespace p5
{
    std::unique_ptr<Shader> loadShader(std::string_view vertexSource, std::string_view fragmentSource)
    {
        return OpenGLShader::create(vertexSource, fragmentSource);
    }
} // namespace p5

namespace p5
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

    std::unique_ptr<Shader> createDefaultShader()
    {
        return loadShader(defaultVSource, defaultFSource);
    }
} // namespace p5

namespace p5
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
} // namespace p5
