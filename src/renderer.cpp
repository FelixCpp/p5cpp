#include "renderer.hpp"

namespace p5
{
    BufferWriteResult GeometryRenderer::convex(const std::span<const GeometryPoint>& points)
    {
        return BufferWriteResult {
            .indexStart = 0,
            .indexCount = 0,
        };
    }

    BufferWriteResult GeometryRenderer::concave(const std::span<const GeometryPoint>& points)
    {
        return BufferWriteResult {
            .indexStart = 0,
            .indexCount = 0,
        };
    }

    BufferWriteResult GeometryRenderer::stroke(const std::span<const GeometryPoint>& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, StrokeAlign strokeAlign, bool closed)
    {
        return BufferWriteResult {
            .indexStart = 0,
            .indexCount = 0,
        };
    }
} // namespace p5

namespace p5
{
    inline static constexpr const char* vShaderSource = R"(
#version 330 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec2 a_TexCoord;
layout (location = 2) in vec4 a_Color;

out vec2 v_TexCoord;
out vec4 v_Color;

uniform mat4 u_ProjectionMatrix;

void main() {
    gl_Position = u_ProjectionMatrix * vec4(a_Position, 1.0);
    v_TexCoord = a_TexCoord;
    v_Color = a_Color;
}
)";

    inline static constexpr const char* fShaderSource = R"(
#version 330 core

layout (location = 0) out vec4 o_Color;

in vec2 v_TexCoord;
in vec4 v_Color;

uniform sampler2D u_Texture;

void main() {
    o_Color = texture(u_Texture, v_TexCoord) * v_Color;
}
)";

    class ImmediateRenderer : public Renderer
    {
    public:
        ImmediateRenderer() : m_vertices(std::make_unique<Vertex[]>(10'000)), m_vertexCount(10'000), m_indices(std::make_unique<uint32_t[]>(30'000)), m_indexCount(30'000)
        {

            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, m_vertexCount * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indexCount * sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(offsetof(Vertex, position)));

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(offsetof(Vertex, texcoord)));

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(offsetof(Vertex, color)));

            glBindVertexArray(0);

            GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vShader, 1, &vShaderSource, nullptr);
            glCompileShader(vShader);

            GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fShader, 1, &fShaderSource, nullptr);
            glCompileShader(fShader);

            shaderProgramId = glCreateProgram();
            glAttachShader(shaderProgramId, vShader);
            glAttachShader(shaderProgramId, fShader);
            glLinkProgram(shaderProgramId);
            glDetachShader(shaderProgramId, vShader);
            glDetachShader(shaderProgramId, fShader);
            glDeleteShader(vShader);
            glDeleteShader(fShader);

            const uint8_t whitePixel[] = {255, 255, 255, 255};
            glGenTextures(1, &whiteTextureId);
            glBindTexture(GL_TEXTURE_2D, whiteTextureId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
        }

        ~ImmediateRenderer() override
        {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);
            glDeleteProgram(shaderProgramId);
            glDeleteTextures(1, &whiteTextureId);
        }

        void beginDraw() override
        {
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            glEnable(GL_BLEND);
            glEnable(GL_TEXTURE_2D);
        }

        void endDraw() override
        {
            glDisable(GL_BLEND);
            glDisable(GL_TEXTURE_2D);
        }

        void draw(const DrawSubmission& submission, const std::function<BufferWriteResult(GeometryRenderer&)>& func) override
        {
            GeometryRenderer renderer = {
                .buffer = WritableBuffer {
                    .vertices = {m_vertices.get(), m_vertices.get() + m_vertexCount},
                    .indices = {m_indices.get(), m_indices.get() + m_indexCount},
                }
            };

            BufferWriteResult result = func(renderer);
            submit(result, submission);
        }

    private:
        void submit(const BufferWriteResult& result, const DrawSubmission& submission)
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * result.vertexCount, m_vertices.get());

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(GLuint) * result.indexCount, m_indices.get());

            // clang-format off
            float l = 0, r = 800, b = 600, t = 0, zn = -1, zf = 1;
            float projectionMatrix[] = {
                2/(r-l),       0,             0,           0,
                0,             2/(t-b),       0,           0,
                0,             0,            -2/(zf-zn),   0,
                -(r+l)/(r-l), -(t+b)/(t-b), -(zf+zn)/(zf-zn), 1,
            };
            // clang-format on

            GLuint program = submission.shaderId.value_or(shaderProgramId);
            GLuint textureId = submission.textureId.value_or(whiteTextureId);

            glUseProgram(program);
            glUniformMatrix4fv(glGetUniformLocation(program, "u_ProjectionMatrix"), 1, GL_FALSE, projectionMatrix);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glUniform1i(glGetUniformLocation(program, "u_Texture"), 0);

            glDrawElements(GL_TRIANGLES, result.indexCount, GL_UNSIGNED_INT, reinterpret_cast<const GLvoid*>(result.indexStart * sizeof(GLuint)));
        }

        GLuint vao;
        GLuint vbo;
        GLuint ebo;
        GLuint shaderProgramId;
        GLuint whiteTextureId;

        std::unique_ptr<Vertex[]> m_vertices;
        size_t m_vertexCount;

        std::unique_ptr<uint32_t[]> m_indices;
        size_t m_indexCount;
    };

    std::unique_ptr<Renderer> createRenderer()
    {
        return std::make_unique<ImmediateRenderer>();
    }
} // namespace p5
