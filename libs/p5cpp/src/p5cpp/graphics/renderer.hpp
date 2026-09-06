#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/vertex_sink.hpp>

#include <glad/glad.h>

#include <optional>
#include <string>
#include <unordered_map>

namespace p5
{
    struct Graphics;

    struct RendererBatch
    {
        BlendMode blendMode;
        std::optional<rect2f> clipRect;
        TextureFilter textureFilter;
        TextureWrap textureWrap;
        Shader shader;
        Texture texture;
        std::unordered_map<std::string, UniformValue> uniforms;
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
            Writer(Renderer& renderer, uint32_t vertexBase, size_t indexOffset);

            Renderer& m_renderer;
            uint32_t m_vertexBase;
            size_t m_indexOffset;
        };

        static std::unique_ptr<Renderer> create(size_t initialMaxVertices, size_t initialMaxIndices);
        ~Renderer();

        void begin(Graphics graphics);
        void end();
        void flush();

        Writer write();
        void finish(const Writer& writer, const BlendMode& blendMode, const std::optional<rect2f>& clipRect, TextureFilter textureFilter, TextureWrap textureWrap, const Texture& texture, const Shader& shader, const std::unordered_map<std::string, UniformValue>& uniforms);

    private:
        explicit Renderer(GLuint vao, GLuint vbo, GLuint ebo, size_t maxVertexCount, size_t maxIndexCount);

        void appendVertex(const Vertex& vertex);
        void appendIndex(uint32_t index);
        // Flushes already-finish()ed batches early if headroom in either buffer has dropped below a
        // reserved margin, so the next shape starts with most of the buffer free again. Only called
        // from write(), between shapes -- see write()'s comment for why that's always safe.
        void flushIfNearCapacity();

        GLuint m_vao;
        GLuint m_vbo;
        GLuint m_ebo;

        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;
        size_t m_uploadedVertexCapacity;
        size_t m_uploadedIndexCapacity;

        size_t m_currentVertexOffset;
        size_t m_currentIndexOffset;

        std::vector<RendererBatch> m_batches;
        matrix4x4 m_projectionMatrix;
        uint2 m_graphicsSize;
        Graphics m_graphics;
    };
} // namespace p5
