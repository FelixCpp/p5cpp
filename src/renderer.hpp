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

        DrawScope aquireDrawScope();

        void beginFrame();
        void endFrame();

        void pushPass(std::shared_ptr<Canvas> canvas);
        void popPass();

        uint2 getCanvasSize() const;
        uint32_t getWhiteTextureId() const { return m_whiteTexture; }

        void submitMesh(const DrawScopeResult& writer, uint32_t texture, std::shared_ptr<Shader> shader, BlendMode blendMode);

    private:
        explicit Renderer(GLuint vao, GLuint vbo, GLuint ebo, GLuint whiteTexture);

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
