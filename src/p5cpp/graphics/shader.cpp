#include <p5cpp/graphics/shader.hpp>
#include <p5cpp/application/logging.hpp>

#include <glad/glad.h>

#include <unordered_map>

namespace p5cpp
{
    class OpenGLShader : public Shader
    {
    public:
        static std::unique_ptr<OpenGLShader> create(std::string_view vertexShaderSource, std::string_view fragmentShaderSource)
        {
            GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
            const char* vertexSource = vertexShaderSource.data();
            glShaderSource(vertexShader, 1, &vertexSource, nullptr);
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

            GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
            const char* fragmentSource = fragmentShaderSource.data();
            glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
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

            GLuint shaderProgram = glCreateProgram();
            glAttachShader(shaderProgram, vertexShader);
            glAttachShader(shaderProgram, fragmentShader);
            glLinkProgram(shaderProgram);

            {
                GLint success = GL_FALSE;
                glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
                if (success == GL_FALSE) {
                    GLint logLength = 0;
                    glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength);
                    std::string log(logLength, '\0');
                    glGetProgramInfoLog(shaderProgram, logLength, nullptr, log.data());
                    error("Failed to link shader program:\n" + log);
                    glDeleteShader(vertexShader);
                    glDeleteShader(fragmentShader);
                    glDeleteProgram(shaderProgram);
                    return nullptr;
                }
            }

            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);

            return std::unique_ptr<OpenGLShader>(new OpenGLShader(shaderProgram));
        }

        ~OpenGLShader() override
        {
            glDeleteProgram(shaderId);
        }

        std::optional<UniformLocation> getUniformLocation(const std::string& name) const override
        {
            const auto itr = uniformLocations.find(name);
            if (itr != uniformLocations.end()) {
                return itr->second;
            }

            const GLint location = glGetUniformLocation(shaderId, name.c_str());
            if (location == -1) {
                return std::nullopt;
            }

            const auto insertion = uniformLocations.emplace(name, UniformLocation {.value = location});
            return insertion.first->second;
        }

        ShaderId getShaderId() const override
        {
            return ShaderId {.value = shaderId};
        }

    private:
        explicit OpenGLShader(GLuint shaderId)
            : shaderId(shaderId)
        {
        }

        GLuint shaderId;
        mutable std::unordered_map<std::string, UniformLocation> uniformLocations;
    };
} // namespace p5cpp

namespace p5cpp
{
    ShaderHandle::ShaderHandle()
        : shader(nullptr)
    {
    }

    std::optional<UniformLocation> ShaderHandle::getUniformLocation(const std::string& name) const
    {
        if (shader) {
            return shader->getUniformLocation(name);
        }

        return std::nullopt;
    }

    ShaderId ShaderHandle::getShaderId() const
    {
        if (shader) {
            return shader->getShaderId();
        }

        return ShaderId {.value = 0};
    }

    ShaderHandle::ShaderHandle(std::unique_ptr<Shader> shader)
        : shader(std::move(shader))
    {
    }
} // namespace p5cpp

namespace p5cpp
{
    std::unique_ptr<Shader> loadShader(std::string_view vertexShaderSource, std::string_view fragmentShaderSource)
    {
        return OpenGLShader::create(vertexShaderSource, fragmentShaderSource);
    }
} // namespace p5cpp
