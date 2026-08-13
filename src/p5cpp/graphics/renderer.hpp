#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/vertex_sink.hpp>

#include <glad/glad.h>

#include <optional>
#include <string>
#include <unordered_map>

namespace p5
{
    struct Framebuffer;

    struct RendererBatch
    {
        BlendMode blendMode;
        std::optional<rect2f> clipRect;
        TextureFilter textureFilter;
        TextureWrap textureWrap;
        std::shared_ptr<Shader> shader;
        std::shared_ptr<Texture> texture;
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

        void begin(std::shared_ptr<Framebuffer> framebuffer);
        void end();
        void flush();

        // Returns a Writer that appends vertices/indices directly into this Renderer's fixed-capacity
        // buffers (sized once at create() and never resized), so callers never need to know a shape's
        // vertex/index count upfront. appendVertex()/appendIndex() throw std::runtime_error if a frame's
        // total geometry exceeds that capacity. Must be followed by exactly one finish() call with the
        // same Writer, which submits whatever was written as a batch (merging into the previous batch if
        // blendMode/shader match).
        Writer write();
        void finish(const Writer& writer, const BlendMode& blendMode, const std::optional<rect2f>& clipRect, TextureFilter textureFilter, TextureWrap textureWrap, const std::shared_ptr<Texture>& texture, const std::shared_ptr<Shader>& shader);

    private:
        explicit Renderer(GLuint vao, GLuint vbo, GLuint ebo, size_t maxVertexCount, size_t maxIndexCount);

        void appendVertex(const Vertex& vertex);
        void appendIndex(uint32_t index);

        GLuint m_vao;
        GLuint m_vbo;
        GLuint m_ebo;

        std::unique_ptr<Vertex[]> m_vertices;
        std::unique_ptr<uint32_t[]> m_indices;

        const size_t m_maxVertexCount;
        const size_t m_maxIndexCount;

        size_t m_currentVertexOffset;
        size_t m_currentIndexOffset;

        std::vector<RendererBatch> m_batches;
        matrix4x4 m_projectionMatrix;
        uint2 m_framebufferSize;
    };
} // namespace p5
