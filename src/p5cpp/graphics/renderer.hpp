#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/vertex_sink.hpp>

#include <glad/glad.h>

namespace p5
{
    class Framebuffer;

    struct RendererSubmission
    {
        std::span<const Vertex> vertices;
        std::span<const uint32_t> indices;
        BlendMode blendMode;
        std::shared_ptr<Shader> shaderProgramId;
        // Texture (does not exist yet) ...
    };

    struct RendererBatch
    {
        BlendMode blendMode;
        GLuint shaderProgramId;
        size_t indexOffset;
        size_t indexCount;
    };

    class Renderer
    {
    public:
        class Writer : public VertexSink
        {
        public:
            void addVertex(const float2& position, const float2& texCoord, const float4& color) override;
            void addIndex(uint32_t index) override;

        private:
            friend class Renderer;
            Writer(Vertex* vertexCursor, uint32_t* indexCursor, uint32_t vertexBase, size_t vertexOffset, size_t indexOffset);

            Vertex* m_vertexCursor;
            uint32_t* m_indexCursor;
            uint32_t m_vertexBase;
            size_t m_vertexOffset;
            size_t m_indexOffset;
            size_t m_vertexCount;
            size_t m_indexCount;
        };

        static std::unique_ptr<Renderer> create(size_t initialMaxVertices, size_t initialMaxIndices);
        ~Renderer();

        void begin(std::shared_ptr<Framebuffer> framebuffer);
        void end();
        void flush();

        void submit(const RendererSubmission& submission);

        // Reserves space for up to maxVertexCount/maxIndexCount vertices/indices and returns a Writer
        // to fill it directly. Must be followed by exactly one finish() call with the same Writer,
        // which shrinks the reservation down to however much was actually written.
        Writer write(size_t maxVertexCount, size_t maxIndexCount, const BlendMode& blendMode, const std::shared_ptr<Shader>& shader);
        void finish(const Writer& writer);

    private:
        struct ReservedRange
        {
            size_t vertexOffset;
            size_t indexOffset;
        };

        explicit Renderer(GLuint vao, GLuint vbo, GLuint ebo, size_t maxVertexCount, size_t maxIndexCount);

        void grow(size_t requiredVertexCount, size_t requiredIndexCount);
        ReservedRange reserve(size_t vertexCount, size_t indexCount, const BlendMode& blendMode, GLuint shaderProgramId);

        GLuint m_vao;
        GLuint m_vbo;
        GLuint m_ebo;

        std::unique_ptr<Vertex[]> m_vertices;
        std::unique_ptr<uint32_t[]> m_indices;

        size_t m_maxVertexCount;
        size_t m_maxIndexCount;

        size_t m_currentVertexOffset;
        size_t m_currentIndexOffset;

        std::vector<RendererBatch> m_batches;
        matrix4x4 m_projectionMatrix;
    };
} // namespace p5
