#pragma once

#include "p5.hpp"
#include "vertex.hpp"
#include "drawscope.hpp"
#include "renderpass.hpp"

#include <glad/glad.h>

#include <memory>
#include <span>
#include <vector>

namespace p5
{
    class Renderer
    {
    public:
        static std::unique_ptr<Renderer> create();

        DrawScope aquireDrawScope();

        void beginFrame(const matrix4x4& projectionMatrix);
        void endFrame(std::span<RenderPass> renderPasses);
        uint32_t getWhiteTextureId() const { return m_whiteTexture; }

        void submitMesh(std::vector<DrawCall>& drawCalls, const DrawScopeResult& writer, uint32_t texture, std::shared_ptr<Shader> shader, BlendMode blendMode);

    private:
        explicit Renderer(GLuint vao, GLuint vbo, GLuint ebo, GLuint whiteTexture);

        std::unique_ptr<Vertex[]> m_vertices;
        std::unique_ptr<uint32_t[]> m_indices;
        uint32_t m_vertexCursor;
        uint32_t m_indexCursor;

        matrix4x4 m_projectionMatrix;

        GLuint m_vao;
        GLuint m_vbo;
        GLuint m_ebo;

        GLuint m_whiteTexture;
    };
} // namespace p5
