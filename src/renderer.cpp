#include "renderer.hpp"

#include <cassert>

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

    void Renderer::beginFrame()
    {
        m_vertexCursor = 0;
        m_indexCursor = 0;
    }

    void Renderer::endFrame()
    {
        if (m_renderPasses.empty()) {
            return;
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertexCursor * sizeof(Vertex), m_vertices.get());

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m_indexCursor * sizeof(uint32_t), m_indices.get());

        glBindVertexArray(m_vao);

        for (const RenderPass& renderPass : m_renderPasses) {
            const uint2 canvasSize = renderPass.canvas->getSize();
            const matrix4x4 orthoProjection = ortho(0.0f, static_cast<float>(canvasSize.y), static_cast<float>(canvasSize.x), 0.0f, -1.0f, 1.0f);

            glBindFramebuffer(GL_FRAMEBUFFER, renderPass.canvas->getRendererId());
            glViewport(0, 0, canvasSize.x, canvasSize.y);

            for (const DrawCall& drawCall : renderPass.drawCalls) {
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
                    glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, orthoProjection.m);
                }

                glDrawElements(GL_TRIANGLES, drawCall.indexCount, GL_UNSIGNED_INT, reinterpret_cast<const GLvoid*>(drawCall.indexOffset * sizeof(uint32_t)));
            }
        }

        m_renderPasses.clear();
        m_activeRenderPassIndex = 0;
    }

    void Renderer::pushPass(std::shared_ptr<Canvas> canvas)
    {
        m_renderPasses.push_back(RenderPass {
            .canvas = std::move(canvas),
            .drawCalls = {},
        });

        m_activeRenderPassIndex = m_renderPasses.size() - 1;
    }

    void Renderer::popPass()
    {
        if (m_activeRenderPassIndex > 0) {
            --m_activeRenderPassIndex;
        }
    }

    uint2 Renderer::getCanvasSize() const
    {
        return m_renderPasses[m_activeRenderPassIndex].canvas->getSize();
    }

    void Renderer::submitMesh(uint32_t baseVertex, uint32_t baseIndex, uint32_t texture, std::shared_ptr<Shader> shader, BlendMode blendMode)
    {
        const uint32_t vertexCount = m_vertexCursor - baseVertex;
        const uint32_t indexCount = m_indexCursor - baseIndex;

        DrawCall& drawCall = getOrCreateDrawCall(std::move(shader), blendMode, texture, baseIndex);
        size_t textureUnitIndex = resolveTExtureUnit(drawCall, texture);

        drawCall.indexCount += indexCount;

        for (size_t i = 0; i < vertexCount; ++i) {
            m_vertices[baseVertex + i].texIndex = static_cast<float>(textureUnitIndex);
        }
    }

    Renderer::Renderer(GLuint vao, GLuint vbo, GLuint ebo, GLuint whiteTexture)
        : m_vertices(std::make_unique<Vertex[]>(MAX_VERTICES)),
          m_indices(std::make_unique<uint32_t[]>(MAX_INDICES)),
          m_vertexCursor(0),
          m_indexCursor(0),
          m_vao(vao),
          m_vbo(vbo),
          m_ebo(ebo),
          m_whiteTexture(whiteTexture)
    {
    }

    DrawCall& Renderer::getOrCreateDrawCall(std::shared_ptr<Shader> shader, BlendMode blendMode, uint32_t texture, uint32_t baseIndex)
    {
        std::vector<DrawCall>& drawCalls = m_renderPasses[m_activeRenderPassIndex].drawCalls;
        DrawCall* previousDrawCall = drawCalls.empty() ? nullptr : &drawCalls.back();

        const bool hasShaderChange = previousDrawCall != nullptr and previousDrawCall->shader != shader;
        const bool hasBlendModeChange = previousDrawCall != nullptr and previousDrawCall->blendMode != blendMode;

        if (previousDrawCall == nullptr or hasShaderChange or hasBlendModeChange) {
            DrawCall newDrawCall = {
                .indexOffset = baseIndex,
                .indexCount = 0,
                .blendMode = blendMode,
                .shader = shader,
                .textureUnits = {},
                .textureUnitCount = 0,
            };

            drawCalls.push_back(newDrawCall);
            return drawCalls.back();
        }

        return *previousDrawCall;
    }

    size_t Renderer::resolveTExtureUnit(DrawCall& drawCall, uint32_t texture)
    {
        for (size_t i = 0; i < drawCall.textureUnitCount; ++i) {
            if (drawCall.textureUnits[i] == texture) {
                return i;
            }
        }

        assert(drawCall.textureUnitCount < drawCall.textureUnits.size() && "Exceeded maximum texture units per draw call");
        const size_t newTextureUnitIndex = drawCall.textureUnitCount;
        drawCall.textureUnits[newTextureUnitIndex] = texture;
        drawCall.textureUnitCount++;
        return newTextureUnitIndex;
    }

    DrawScope Renderer::createDrawScope()
    {
        return DrawScope {
            .vertices = std::span {m_vertices.get(), MAX_VERTICES},
            .indices = std::span {m_indices.get(), MAX_INDICES},
            .vertexCursor = m_vertexCursor,
            .indexCursor = m_indexCursor,
            .baseVertex = m_vertexCursor,
            .baseIndex = m_indexCursor,
        };
    }
} // namespace p5
