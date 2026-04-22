#include "p5_internal.hpp"

#include <algorithm>

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

namespace p5
{
    static bool operator<(const DrawCommandKey& lhs, const DrawCommandKey& rhs)
    {
        if (lhs.layer != rhs.layer) return lhs.layer < rhs.layer;
        if (lhs.blendMode != rhs.blendMode) return static_cast<int>(lhs.blendMode) < static_cast<int>(rhs.blendMode);
        if (lhs.shaderId != rhs.shaderId) return lhs.shaderId < rhs.shaderId;
        return lhs.textureId < rhs.textureId;
    }

    static bool operator==(const DrawCommandKey& lhs, const DrawCommandKey& rhs)
    {
        return lhs.layer == rhs.layer and lhs.blendMode == rhs.blendMode and lhs.shaderId == rhs.shaderId and lhs.textureId == rhs.textureId;
    }

    static void activate(BlendMode blendMode)
    {
        switch (blendMode) {
            case BlendMode::alpha:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                break;
            case BlendMode::additive:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                break;
            case BlendMode::multiply:
                glBlendFunc(GL_DST_COLOR, GL_ZERO);
                break;
        }
    }
} // namespace p5

namespace p5
{
    std::unique_ptr<Renderer> Renderer::create(GeometryBuffer& buffer, size_t maxBatchSize)
    {
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, buffer.vertexCount * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

        GLuint ebo;
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, buffer.indexCount * sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);

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

        GLuint shaderProgramId = glCreateProgram();
        glAttachShader(shaderProgramId, vShader);
        glAttachShader(shaderProgramId, fShader);
        glLinkProgram(shaderProgramId);
        glDetachShader(shaderProgramId, vShader);
        glDetachShader(shaderProgramId, fShader);
        glDeleteShader(vShader);
        glDeleteShader(fShader);

        const uint8_t whitePixel[] = {255, 255, 255, 255};
        GLuint whiteTextureId;
        glGenTextures(1, &whiteTextureId);
        glBindTexture(GL_TEXTURE_2D, whiteTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);

        auto renderer = new Renderer();
        renderer->m_vertexArrayId = vao;
        renderer->m_vertexBufferId = vbo;
        renderer->m_indexBufferId = ebo;
        renderer->m_shaderProgramId = shaderProgramId;
        renderer->m_whiteTextureId = whiteTextureId;
        renderer->m_maxBatchSize = maxBatchSize;
        renderer->m_vertexOffset = 0;
        renderer->m_indexOffset = 0;
        renderer->m_commandOffset = 0;
        renderer->m_buffer = &buffer;
        renderer->m_commands = std::make_unique<DrawCommand[]>(maxBatchSize);
        renderer->m_submissionCounter = 0;

        return std::unique_ptr<Renderer>(renderer);
    }

    Renderer::~Renderer()
    {
        glDeleteVertexArrays(1, &m_vertexArrayId);
        glDeleteBuffers(1, &m_vertexBufferId);
        glDeleteBuffers(1, &m_indexBufferId);
        glDeleteProgram(m_shaderProgramId);
        glDeleteTextures(1, &m_whiteTextureId);
    }

    void Renderer::beginDraw()
    {
        glEnable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
    }

    void Renderer::endDraw()
    {
        flush();

        glDisable(GL_BLEND);
        glDisable(GL_TEXTURE_2D);
    }

    void Renderer::flush()
    {
        if (m_commandOffset == 0) {
            return;
        }

        std::sort(m_commands.get(), m_commands.get() + m_commandOffset, [](const DrawCommand& lhs, const DrawCommand& rhs) {
            return lhs.key < rhs.key;
        });

        glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferId);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * m_vertexOffset, m_buffer->vertices.get());

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferId);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(GLuint) * m_indexOffset, m_buffer->indices.get());

        glBindVertexArray(m_vertexArrayId);

        // clang-format off
        float l = 0, r = 800, b = 600, t = 0, zn = -1, zf = 1;
        float projectionMatrix[] = {
            2/(r-l),       0,             0,           0,
            0,             2/(t-b),       0,           0,
            0,             0,            -2/(zf-zn),   0,
            -(r+l)/(r-l), -(t+b)/(t-b), -(zf+zn)/(zf-zn), 1,
        };
        // clang-format on

        for (size_t i = 0; i < m_commandOffset; ++i) {
            const DrawCommand& command = m_commands[i];

            glUseProgram(command.key.shaderId);
            glUniformMatrix4fv(glGetUniformLocation(command.key.shaderId, "u_ProjectionMatrix"), 1, GL_FALSE, projectionMatrix);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, command.key.textureId);
            glUniform1i(glGetUniformLocation(command.key.shaderId, "u_Texture"), 0);

            activate(command.key.blendMode);

            glDrawElements(GL_TRIANGLES, command.indexCount, GL_UNSIGNED_INT, reinterpret_cast<const GLvoid*>(command.indexOffset * sizeof(GLuint)));
        }

        m_vertexOffset = 0;
        m_indexOffset = 0;
        m_commandOffset = 0;
        m_submissionCounter = 0;
    }

    void Renderer::submit(const GeometryBuilder& builder, const DrawSubmission& submission)
    {
        if (needsFlush(builder.vertexCount, builder.indexCount, 1)) flush();

        const std::span<const Vertex> vertices = {m_buffer->vertices.get() + builder.initialVertexOffset, m_buffer->vertices.get() + builder.initialVertexOffset + builder.vertexCount};
        const std::span<const uint32_t> indices = {m_buffer->indices.get() + builder.initialIndexOffset, m_buffer->indices.get() + builder.initialIndexOffset + builder.indexCount};

        m_vertexOffset += builder.vertexCount;
        m_indexOffset += builder.indexCount;

        const DrawCommandKey key = generateKey(submission);

        if (m_commandOffset > 0) {
            DrawCommand& lastCommand = m_commands[m_commandOffset - 1];
            if (lastCommand.key == key) {
                lastCommand.indexCount += builder.indexCount;
                return;
            }
        }

        m_commands[m_commandOffset++] = DrawCommand {
            .key = key,
            .indexOffset = builder.initialIndexOffset,
            .indexCount = builder.indexCount,
        };
    }

    GeometryBuilder Renderer::getNextBuilder()
    {
        return GeometryBuilder {
            .initialVertexOffset = m_vertexOffset,
            .initialIndexOffset = m_indexOffset,
            .vertexCount = 0,
            .indexCount = 0,
            .buffer = m_buffer,
        };
    }

    bool Renderer::needsFlush(size_t additionalVertices, size_t additionalIndices, size_t additionalBatches)
    {
        if (m_vertexOffset + additionalVertices >= m_buffer->vertexCount) {
            return true;
        }

        if (m_indexOffset + additionalIndices >= m_buffer->indexCount) {
            return true;
        }

        if (m_commandOffset + additionalBatches >= m_maxBatchSize) {
            return true;
        }

        return false;
    }

    DrawCommandKey Renderer::generateKey(const DrawSubmission& submission)
    {
        return DrawCommandKey {
            .layer = m_submissionCounter++,
            .blendMode = submission.blendMode,
            .shaderId = submission.shaderId.value_or(m_shaderProgramId),
            .textureId = submission.textureId.value_or(m_whiteTextureId),
        };
    }
} // namespace p5
