#include <p5cpp/graphics/shader.hpp>
#include <p5cpp/application/logging.hpp>

#include <glad/glad.h>

#include <unordered_map>

namespace p5cpp::detail
{
    struct ShaderResource
    {
        GLuint programId;
        mutable std::unordered_map<std::string, UniformLocation> uniformLocations;

        explicit ShaderResource(GLuint programId)
            : programId(programId)
        {
        }

        ~ShaderResource()
        {
            glDeleteProgram(programId);
        }
    };
} // namespace p5cpp::detail

namespace
{
    std::shared_ptr<p5cpp::detail::ShaderResource> compileShader(std::string_view vertexShaderSource, std::string_view fragmentShaderSource)
    {
        using namespace p5cpp;

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

        return std::make_shared<p5cpp::detail::ShaderResource>(shaderProgram);
    }

    std::string makeShaderCacheKey(std::string_view vertexShaderSource, std::string_view fragmentShaderSource)
    {
        std::string key;
        key.reserve(vertexShaderSource.size() + fragmentShaderSource.size() + 1);
        key.append(vertexShaderSource);
        key.push_back('\x1f');
        key.append(fragmentShaderSource);
        return key;
    }

    std::unordered_map<std::string, std::weak_ptr<p5cpp::detail::ShaderResource>> s_shaderCache;
} // namespace

namespace p5cpp
{
    // loadShader(vertexSrc, fragmentSrc) caches by source pair: a repeated call for the
    // same source returns a Shader that aliases the already-compiled program (via the
    // shared_ptr this Shader already carries for RAII) instead of recompiling/relinking.
    Shader loadShader(std::string_view vertexShaderSource, std::string_view fragmentShaderSource)
    {
        const std::string key = makeShaderCacheKey(vertexShaderSource, fragmentShaderSource);

        if (const auto it = s_shaderCache.find(key); it != s_shaderCache.end()) {
            if (std::shared_ptr<detail::ShaderResource> cached = it->second.lock()) {
                Shader shader;
                shader.id = ShaderId {.value = cached->programId};
                shader.resource = std::move(cached);
                return shader;
            }
        }

        std::shared_ptr<detail::ShaderResource> fresh = compileShader(vertexShaderSource, fragmentShaderSource);
        if (!fresh) {
            return Shader();
        }

        s_shaderCache[key] = fresh;

        Shader shader;
        shader.id = ShaderId {.value = fresh->programId};
        shader.resource = std::move(fresh);
        return shader;
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

    Shader loadEffectShader(std::string_view effectSource)
    {
        const std::string fragmentSource = std::string(effectFHeader) + std::string(effectSource) + std::string(effectFFooter);
        return loadShader(effectVSource, fragmentSource);
    }
} // namespace p5cpp

namespace p5cpp
{
    bool isShaderValid(const Shader& shader)
    {
        return shader.resource != nullptr;
    }

    std::optional<UniformLocation> getUniformLocation(const Shader& shader, const std::string& name)
    {
        if (!shader.resource) {
            return std::nullopt;
        }

        const auto itr = shader.resource->uniformLocations.find(name);
        if (itr != shader.resource->uniformLocations.end()) {
            return itr->second;
        }

        const GLint location = glGetUniformLocation(shader.resource->programId, name.c_str());
        if (location == -1) {
            return std::nullopt;
        }

        const auto insertion = shader.resource->uniformLocations.emplace(name, UniformLocation {.value = location});
        return insertion.first->second;
    }
} // namespace p5cpp
