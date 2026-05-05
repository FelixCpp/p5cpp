#include "shader.hpp"
#include <string>

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
    };

    std::unique_ptr<Shader> loadShader(std::string_view vertexSource, std::string_view fragmentSource)
    {
        return OpenGLShader::create(vertexSource, fragmentSource);
    }
} // namespace p5

namespace p5
{

    inline static constexpr const char* vSource = R"(
  #version 330 core

  layout (location = 0) in vec2 a_Position;
  layout (location = 1) in vec2 a_TexCoord;
  layout (location = 2) in vec4 a_Color;

  out vec2 v_TexCoord;
  out vec4 v_Color;

  uniform mat4 u_ProjectionMatrix;

  void main() {
    gl_Position = u_ProjectionMatrix * vec4(a_Position, 0.0, 1.0);
    v_TexCoord = a_TexCoord;
    v_Color = a_Color;
  }
)";

    inline static constexpr const char* fSource = R"(
  #version 330 core

  layout (location = 0) out vec4 o_Color;

  in vec2 v_TexCoord;
  in vec4 v_Color;

  uniform sampler2D u_Texture;

  void main() {
    o_Color = texture(u_Texture, v_TexCoord) * v_Color;
  }
)";

    std::unique_ptr<Shader> createDefaultShader()
    {
        return loadShader(vSource, fSource);
    }
} // namespace p5

namespace p5
{
    inline static constexpr const char* textVSource = R"(
        #version 330 core

        layout (location = 0) in vec2 a_Position;
        layout (location = 1) in vec2 a_TexCoord;
        layout (location = 2) in vec4 a_Color;

        out vec2 v_TexCoord;
        out vec4 v_Color;

        uniform mat4 u_ProjectionMatrix;

        void main() {
            gl_Position = u_ProjectionMatrix * vec4(a_Position, 0.0, 1.0);
            v_TexCoord = a_TexCoord;
            v_Color = a_Color;
        }
    )";

    inline static constexpr const char* textFSource = R"(
        #version 330 core

        layout (location = 0) out vec4 o_Color;

        in vec2 v_TexCoord;
        in vec4 v_Color;

        uniform sampler2D u_Texture;

        void main() {
            float alpha = texture(u_Texture, v_TexCoord).r;
            o_Color = vec4(v_Color.rgb, v_Color.a * alpha);
        }
    )";

    std::unique_ptr<Shader> createTextShader()
    {
        return loadShader(textVSource, textFSource);
    }
} // namespace p5
