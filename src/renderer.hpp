#pragma once

#include "vertex.hpp"
#include "meshwriter.hpp"

#include <glad/glad.h>

#include <memory>
#include <vector>
#include <array>

namespace p5
{

    struct DrawCall
    {
        uint32_t indexOffset;
        uint32_t indexCount;

        BlendMode blendMode;

        std::shared_ptr<Shader> shader;

        std::array<GLuint, 8> textureUnits;
        size_t textureUnitCount;
    };

    struct BatchState
    {
        BlendMode blendMode;
        std::shared_ptr<Shader> shader;

        std::array<GLuint, 8> textureUnits;
        uint8_t textureUnitCount;

        int getOrAssignSlot(GLuint texture);
        bool wouldNeedBreak(GLuint texture);
        bool breaksWith(BlendMode blendMode, Shader* shader);
    };

    class Renderer
    {
    public:
        static std::unique_ptr<Renderer> create();

        MeshWriter aquireMeshWriter();

        void beginFrame(const matrix4x4& projectionMatrix);
        void endFrame();

        void submitMesh(MeshWriter& meshWriter, GLuint texture, std::shared_ptr<Shader> shader, BlendMode blendMode);

    private:
        explicit Renderer(GLuint vao, GLuint vbo, GLuint ebo, GLuint whiteTexture);

        std::unique_ptr<Vertex[]> m_vertices;
        std::unique_ptr<uint32_t[]> m_indices;
        uint32_t m_vertexCursor;
        uint32_t m_indexCursor;

        std::vector<DrawCall> m_drawCalls;
        BatchState m_currentBatchState;

        matrix4x4 m_projectionMatrix;

        GLuint m_vao;
        GLuint m_vbo;
        GLuint m_ebo;

        GLuint m_whiteTexture;
    };
} // namespace p5
