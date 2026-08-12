#include <p5cpp/p5cpp.hpp>

#include <glad/glad.h>

#include <optional>
#include <iostream>

namespace p5
{
    namespace
    {
        std::optional<GLuint> createShader(std::string_view source, GLenum type)
        {
            const char* sourceCStr = source.data();

            GLuint shader = glCreateShader(type);
            glShaderSource(shader, 1, &sourceCStr, nullptr);
            glCompileShader(shader);

            GLint success = GL_FALSE;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

            if (success != GL_TRUE) {
                GLint logLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

                std::string log(logLength, '\0');
                glGetShaderInfoLog(shader, logLength, nullptr, log.data());
                std::cerr << "Shader compilation failed: " << log << std::endl;

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
                std::cerr << "Shader compilation failed: " << log << std::endl;

                glDeleteProgram(program);
                glDeleteShader(*vertexShader);
                glDeleteShader(*fragmentShader);
                return std::nullopt;
            }

            glDeleteShader(*vertexShader);
            glDeleteShader(*fragmentShader);

            return program;
        }
    } // namespace

    class OpenGLShader : public Shader
    {
    public:
        static std::unique_ptr<OpenGLShader> create(std::string_view vertexShaderSource, std::string_view fragmentShaderSource)
        {
            const auto shaderProgramId = createShaderProgram(vertexShaderSource, fragmentShaderSource);
            if (not shaderProgramId.has_value()) {
                return nullptr;
            }

            return std::unique_ptr<OpenGLShader>(new OpenGLShader(shaderProgramId.value()));
        }

        uint32_t getShaderProgramId() const override
        {
            return m_shaderProgramId;
        }

    private:
        explicit OpenGLShader(uint32_t shaderProgramId)
            : m_shaderProgramId(shaderProgramId)
        {
        }

        uint32_t m_shaderProgramId;
    };
} // namespace p5

namespace p5
{
    std::unique_ptr<Shader> loadShaderFromMemory(std::string_view vertexShaderSource, std::string_view fragmentShaderSource)
    {
        return OpenGLShader::create(vertexShaderSource, fragmentShaderSource);
    }
} // namespace p5
