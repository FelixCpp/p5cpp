#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/default_shaders.hpp>

#include <glad/glad.h>

#include <optional>
#include <string>
#include <unordered_map>

namespace p5
{
    namespace
    {
        std::optional<GLuint> createShader(std::string_view source, GLenum type)
        {
            const char* sourceCStr = source.data();
            const GLint sourceLength = static_cast<GLint>(source.size());

            GLuint shader = glCreateShader(type);
            glShaderSource(shader, 1, &sourceCStr, &sourceLength);
            glCompileShader(shader);

            GLint success = GL_FALSE;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

            if (success != GL_TRUE) {
                GLint logLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

                std::string log(logLength, '\0');
                glGetShaderInfoLog(shader, logLength, nullptr, log.data());
                error("Shader compilation failed: {}", log);

                glDeleteShader(shader);
                return std::nullopt;
            }

            return shader;
        }

        std::optional<GLuint> createShaderProgram(std::string_view vertexShaderSource, std::string_view fragmentShaderSource)
        {
            auto vertexShader = createShader(vertexShaderSource, GL_VERTEX_SHADER);
            if (not vertexShader.has_value()) {
                return std::nullopt;
            }

            auto fragmentShader = createShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
            if (not fragmentShader.has_value()) {
                glDeleteShader(*vertexShader);
                return std::nullopt;
            }

            GLuint program = glCreateProgram();
            glAttachShader(program, *vertexShader);
            glAttachShader(program, *fragmentShader);
            glLinkProgram(program);

            GLint success = GL_FALSE;
            glGetProgramiv(program, GL_LINK_STATUS, &success);

            if (success != GL_TRUE) {
                GLint logLength = 0;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

                std::string log(logLength, '\0');
                glGetProgramInfoLog(program, logLength, nullptr, log.data());
                error("Shader linking failed: {}", log);

                glDeleteProgram(program);
                glDeleteShader(*vertexShader);
                glDeleteShader(*fragmentShader);
                return std::nullopt;
            }

            glDeleteShader(*vertexShader);
            glDeleteShader(*fragmentShader);

            return program;
        }

        inline static constexpr std::string_view effectFragmentHeaderSource = R"(
            #version 410

            layout (location = 0) out vec4 o_FragColor;

            in vec2 v_TexCoord;
            in vec4 v_Color;

            uniform sampler2D u_Texture;
        )";

        inline static constexpr std::string_view effectFragmentFooterSource = R"(
            void main()
            {
                o_FragColor = effect(v_Color, u_Texture, v_TexCoord, gl_FragCoord.xy);
            }
        )";
    } // namespace

} // namespace p5

namespace p5
{
    Shader::~Shader()
    {
        glDeleteProgram(programId);
    }

    int32_t getUniformLocation(Shader& shader, std::string_view name)
    {
        const std::string key(name);
        const auto it = shader.uniformLocationCache.find(key);
        if (it != shader.uniformLocationCache.end()) {
            return it->second;
        }

        const GLint location = glGetUniformLocation(shader.programId, key.c_str());
        shader.uniformLocationCache.emplace(key, location);
        return location;
    }

    std::unique_ptr<Shader> loadShaderFromMemory(std::string_view vertexShaderSource, std::string_view fragmentShaderSource)
    {
        const auto shaderProgramId = createShaderProgram(vertexShaderSource, fragmentShaderSource);
        if (not shaderProgramId.has_value()) {
            return nullptr;
        }

        auto shader = std::make_unique<Shader>();
        shader->programId = shaderProgramId.value();
        return shader;
    }

    std::unique_ptr<Shader> loadShaderFromMemory(std::string_view effectSource)
    {
        std::string fragmentSource;
        fragmentSource.reserve(effectFragmentHeaderSource.size() + effectSource.size() + effectFragmentFooterSource.size());
        fragmentSource += effectFragmentHeaderSource;
        fragmentSource += effectSource;
        fragmentSource += effectFragmentFooterSource;

        return loadShaderFromMemory(detail::defaultVertexShaderSource, fragmentSource);
    }

    void setUniform(Shader& shader, std::string_view name, float value)
    {
        shader.uniforms[std::string(name)] = value;
    }

    void setUniform(Shader& shader, std::string_view name, const float2& value)
    {
        shader.uniforms[std::string(name)] = value;
    }

    void setUniform(Shader& shader, std::string_view name, const float3& value)
    {
        shader.uniforms[std::string(name)] = value;
    }

    void setUniform(Shader& shader, std::string_view name, const float4& value)
    {
        shader.uniforms[std::string(name)] = value;
    }

    void setUniform(Shader& shader, std::string_view name, const matrix4x4& value)
    {
        shader.uniforms[std::string(name)] = value;
    }
} // namespace p5
