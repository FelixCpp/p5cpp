#pragma once

#include "p5.hpp"
#include "vertex.hpp"
#include "drawscope.hpp"

#include <glad/glad.h>

#include <memory>
#include <vector>

namespace p5
{
    struct DrawCall
    {
        size_t indexOffset;
        size_t indexCount;

        BlendMode blendMode;

        std::shared_ptr<Shader> shader;

        std::array<uint32_t, 8> textureUnits;
        size_t textureUnitCount;
    };

    class Renderer
    {
    public:
        static std::unique_ptr<Renderer> create();

        void submit(std::shared_ptr<Shader> shader, BlendMode blendMode, uint32_t texture, std::invocable<DrawScope&> auto&& function)
        {
            DrawScope scope = createDrawScope();
            function(scope);
            submitMesh(scope.baseVertex, scope.baseIndex, texture, std::move(shader), blendMode);
        }

        void beginFrame();
        void endFrame();

        void pushPass(std::shared_ptr<Canvas> canvas);
        void popPass();

        uint2 getCanvasSize() const;
        uint32_t getWhiteTextureId() const { return m_whiteTexture; }

    private:
        explicit Renderer(GLuint vao, GLuint vbo, GLuint ebo, GLuint whiteTexture);

        DrawCall& getOrCreateDrawCall(std::shared_ptr<Shader> shader, BlendMode blendMode, uint32_t texture, uint32_t baseIndex);
        size_t resolveTExtureUnit(DrawCall& drawCall, uint32_t texture);

        DrawScope createDrawScope();
        void submitMesh(uint32_t baseVertex, uint32_t baseIndex, uint32_t texture, std::shared_ptr<Shader> shader, BlendMode blendMode);

        struct RenderPass
        {
            std::shared_ptr<Canvas> canvas;
            std::vector<DrawCall> drawCalls;
        };

        std::unique_ptr<Vertex[]> m_vertices;
        std::unique_ptr<uint32_t[]> m_indices;
        uint32_t m_vertexCursor;
        uint32_t m_indexCursor;

        std::vector<RenderPass> m_renderPasses;
        size_t m_activeRenderPassIndex;

        GLuint m_vao;
        GLuint m_vbo;
        GLuint m_ebo;

        GLuint m_whiteTexture;
    };
} // namespace p5
