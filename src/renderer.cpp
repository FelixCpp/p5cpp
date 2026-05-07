#include "renderer.hpp"

#include <cassert>
#include <optional>

inline static constexpr size_t MAX_VERTICES = 65536;
inline static constexpr size_t MAX_INDICES = MAX_VERTICES * 3;

namespace p5
{
    void setBlendMode(BlendMode mode)
    {
        // clang-format off
        switch (mode) {
            case BlendMode::none: glDisable(GL_BLEND); break;
            case BlendMode::alpha: glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
            case BlendMode::additive: glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); break;
            case BlendMode::multiply: glEnable(GL_BLEND); glBlendFunc(GL_DST_COLOR, GL_ZERO); break;
        }
        // clang-format on
    }
} // namespace p5

namespace p5
{
    std::unique_ptr<Renderer> Renderer::create()
    {
        GLuint vao = 0;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        GLuint vbo = 0;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

        GLuint ebo = 0;
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_INDICES * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texIndex));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        static constexpr uint8_t whitePixel[] = {255, 255, 255, 255};
        GLuint whiteTexture = 0;
        glGenTextures(1, &whiteTexture);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        return std::unique_ptr<Renderer>(new Renderer(vao, vbo, ebo, whiteTexture));
    }

    DrawScope Renderer::aquireDrawScope()
    {
        return DrawScope(
            std::span {m_vertices.get(), MAX_VERTICES},
            std::span {m_indices.get(), MAX_INDICES},
            m_vertexCursor,
            m_indexCursor
        );
    }

    void Renderer::beginFrame(const matrix4x4& projectionMatrix)
    {
        m_projectionMatrix = projectionMatrix;
        m_vertexCursor = 0;
        m_indexCursor = 0;
    }

    void Renderer::endFrame()
    {
        if (m_drawCalls.empty()) {
            return;
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertexCursor * sizeof(Vertex), m_vertices.get());

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m_indexCursor * sizeof(uint32_t), m_indices.get());

        glBindVertexArray(m_vao);

        for (const DrawCall& drawCall : m_drawCalls) {
            glUseProgram(drawCall.shader->getRendererId());
            setBlendMode(drawCall.blendMode);

            for (size_t i = 0; i < drawCall.textureUnitCount; ++i) {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, drawCall.textureUnits[i]);
            }

            static constexpr int samplers[] = {0, 1, 2, 3, 4, 5, 6, 7};
            if (const GLint samplersLocation = drawCall.shader->getUniformLocation("u_Textures"); samplersLocation != -1) {
                glUniform1iv(samplersLocation, static_cast<GLsizei>(drawCall.textureUnitCount), samplers);
            }

            if (const GLint projectionMatrixLocation = drawCall.shader->getUniformLocation("u_ProjectionMatrix"); projectionMatrixLocation != -1) {
                glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, m_projectionMatrix.m);
            }

            glDrawElements(GL_TRIANGLES, drawCall.indexCount, GL_UNSIGNED_INT, (void*)(drawCall.indexOffset * sizeof(uint32_t)));
        }
    }

    void Renderer::submitMesh(const DrawScopeResult& result, uint32_t texture, std::shared_ptr<Shader> shader, BlendMode blendMode)
    {
        if (m_drawCalls.empty()) {
            DrawCall drawCall = {
                .indexOffset = result.baseIndex,
                .indexCount = 0,
                .blendMode = blendMode,
                .shader = shader,
                .textureUnits = {},
                .textureUnitCount = 0,
            };

            m_drawCalls.push_back(drawCall);
        } else {
            DrawCall& drawCall = m_drawCalls.back();
            const bool hasShaderChange = drawCall.shader != shader;
            const bool hasBlendModeChange = drawCall.blendMode != blendMode;
            const bool hasTextureChange = std::invoke([&]() {
                for (size_t i = 0; i < drawCall.textureUnitCount; ++i) {
                    if (drawCall.textureUnits[i] == texture) {
                        return false;
                    }
                }

                return true;
            });
            const bool textureSlotAvailable = drawCall.textureUnitCount < drawCall.textureUnits.size();
            const bool canBatch = not hasShaderChange and not hasBlendModeChange and (not hasTextureChange or textureSlotAvailable);

            if (not canBatch) {
                DrawCall newDrawCall = {
                    .indexOffset = result.baseIndex,
                    .indexCount = 0,
                    .blendMode = blendMode,
                    .shader = shader,
                    .textureUnits = {},
                    .textureUnitCount = 0,
                };

                m_drawCalls.push_back(newDrawCall);
            }
        }

        DrawCall& drawCall = m_drawCalls.back();
        drawCall.indexCount += result.indexCount;

        std::optional<size_t> foundTextureUnitIndex;
        for (size_t i = 0; i < drawCall.textureUnitCount; ++i) {
            if (drawCall.textureUnits[i] == texture) {
                foundTextureUnitIndex = i;
                break;
            }
        }

        const size_t textureUnitIndex = foundTextureUnitIndex.value_or(drawCall.textureUnitCount);

        if (not foundTextureUnitIndex.has_value()) {
            drawCall.textureUnits[textureUnitIndex] = texture;
            drawCall.textureUnitCount++;
        }

        for (size_t i = 0; i < result.vertexCount; ++i) {
            m_vertices[result.baseVertex + i].texIndex = static_cast<float>(textureUnitIndex);
        }

        m_vertexCursor += result.vertexCount;
        m_indexCursor += result.indexCount;
    }

    Renderer::Renderer(GLuint vao, GLuint vbo, GLuint ebo, GLuint whiteTexture)
        : m_vertices(std::make_unique<Vertex[]>(MAX_VERTICES)),
          m_indices(std::make_unique<uint32_t[]>(MAX_INDICES)),
          m_vertexCursor(0),
          m_indexCursor(0),
          m_projectionMatrix(),
          m_vao(vao),
          m_vbo(vbo),
          m_ebo(ebo),
          m_whiteTexture(whiteTexture)
    {
    }
} // namespace p5
