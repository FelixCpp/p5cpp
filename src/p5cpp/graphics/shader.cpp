#include <p5cpp/graphics/shader.hpp>
#include <p5cpp/application/logging.hpp>

#include <glad/glad.h>

#include <unordered_map>

namespace p5cpp
{
    class OpenGLShaderImpl : public ShaderImpl
    {
    public:
        static std::unique_ptr<OpenGLShaderImpl> create(std::string_view vertexShaderSource, std::string_view fragmentShaderSource)
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

            return std::unique_ptr<OpenGLShaderImpl>(new OpenGLShaderImpl(shaderProgram));
        }

        ~OpenGLShaderImpl() override
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
        explicit OpenGLShaderImpl(GLuint shaderId)
            : shaderId(shaderId)
        {
        }

        GLuint shaderId;
        mutable std::unordered_map<std::string, UniformLocation> uniformLocations;
    };
} // namespace p5cpp

namespace
{
    // Forwards every call to a shared, already-compiled ShaderImpl so that repeated
    // loadShader(vertexSrc, fragmentSrc) calls for the same source pair can return a
    // lightweight handle instead of recompiling/relinking, while still handing back a
    // genuinely unique_ptr-owned object at each call site. ShaderImpl is read-only
    // after construction, so aliasing is safe.
    class SharedShaderImpl : public p5cpp::ShaderImpl
    {
    public:
        explicit SharedShaderImpl(std::shared_ptr<p5cpp::ShaderImpl> shared) : m_shared(std::move(shared)) {}

        std::optional<p5cpp::UniformLocation> getUniformLocation(const std::string& name) const override { return m_shared->getUniformLocation(name); }
        p5cpp::ShaderId getShaderId() const override { return m_shared->getShaderId(); }

    private:
        std::shared_ptr<p5cpp::ShaderImpl> m_shared;
    };

    std::string makeShaderCacheKey(std::string_view vertexShaderSource, std::string_view fragmentShaderSource)
    {
        std::string key;
        key.reserve(vertexShaderSource.size() + fragmentShaderSource.size() + 1);
        key.append(vertexShaderSource);
        key.push_back('\x1f');
        key.append(fragmentShaderSource);
        return key;
    }

    std::unordered_map<std::string, std::weak_ptr<p5cpp::ShaderImpl>> s_shaderCache;
} // namespace

namespace p5cpp
{
    std::unique_ptr<ShaderImpl> loadShader(std::string_view vertexShaderSource, std::string_view fragmentShaderSource)
    {
        const std::string key = makeShaderCacheKey(vertexShaderSource, fragmentShaderSource);

        if (const auto it = s_shaderCache.find(key); it != s_shaderCache.end()) {
            if (std::shared_ptr<ShaderImpl> cached = it->second.lock()) {
                return std::make_unique<SharedShaderImpl>(std::move(cached));
            }
        }

        std::shared_ptr<ShaderImpl> fresh = OpenGLShaderImpl::create(vertexShaderSource, fragmentShaderSource);
        if (!fresh) {
            return nullptr;
        }

        s_shaderCache[key] = fresh;
        return std::make_unique<SharedShaderImpl>(std::move(fresh));
    }
} // namespace p5cpp

namespace p5cpp
{
    // Every effect pass draws the same plain fullscreen quad, so custom effect shaders
    // can all share this vertex shader instead of every caller redefining it.
    inline static constexpr const char* effectVSource = R"(
        #version 410 core

        layout (location = 0) in vec2 a_Position;
        layout (location = 1) in vec2 a_TexCoord;
        layout (location = 2) in vec4 a_Color;
        layout (location = 3) in float a_TexIndex;

        out vec2 v_TexCoord;
        out vec4 v_Color;

        uniform mat4 u_ProjectionMatrix;

        void main() {
            gl_Position = u_ProjectionMatrix * vec4(a_Position, 0.0, 1.0);
            v_TexCoord = a_TexCoord;
            v_Color = a_Color;
        }
    )";

    inline static constexpr const char* effectFHeader = R"(
        #version 410 core

        layout (location = 0) out vec4 o_Color;

        in vec2 v_TexCoord;
        in vec4 v_Color;

        uniform sampler2D u_Textures[8];
        uniform vec2 u_TexelSize;
    )";

    // effect() always binds the source canvas to texture slot 0, so `u_Textures[0]` is
    // a constant index here (no need for the v_TexIndex switch the default/text/blur
    // shaders use to pick between up to 8 dynamically-bound textures).
    inline static constexpr const char* effectFFooter = R"(
        void main() {
            o_Color = v_Color * effect(v_TexCoord, u_TexelSize, u_Textures[0]);
        }
    )";

    std::unique_ptr<ShaderImpl> loadEffectShader(std::string_view effectSource)
    {
        const std::string fragmentSource = std::string(effectFHeader) + std::string(effectSource) + std::string(effectFFooter);
        return loadShader(effectVSource, fragmentSource);
    }
} // namespace p5cpp

namespace p5cpp
{
    Shader::Shader()
        : shader(nullptr)
    {
    }

    Shader::Shader(std::unique_ptr<ShaderImpl> shader)
        : shader(std::move(shader))
    {
    }

    Shader::Shader(std::shared_ptr<ShaderImpl> shader)
        : shader(std::move(shader))
    {
    }

    std::optional<UniformLocation> Shader::getUniformLocation(const std::string& name) const
    {
        if (shader) {
            return shader->getUniformLocation(name);
        }

        return std::nullopt;
    }

    ShaderId Shader::getShaderId() const
    {
        if (shader) {
            return shader->getShaderId();
        }

        return ShaderId {.value = 0};
    }
} // namespace p5cpp
