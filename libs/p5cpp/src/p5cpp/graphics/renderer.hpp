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

        // Returns a Writer that appends vertices/indices directly into this Renderer's fixed-capacity
        // buffers (sized once at create() and never resized), so callers never need to know a shape's
        // vertex/index count upfront. write() may itself call flush() first if the buffers are getting
        // low on room (see flushIfNearCapacity()) -- always safe here because write() is only ever
        // called between shapes, never while a previous Writer is still being filled, so there is
        // never not-yet-finish()ed geometry for a flush to strand. That headroom margin is a heuristic
        // sized for shapes whose vertex count this library itself bounds (a handful of points, or an
        // ellipse/arc/rounded-rect segment count capped at 256) -- it is NOT a hard guarantee against a
        // single shape overrunning the buffer outright, since a big enough shape can still exceed
        // whatever fraction of the buffer the margin reserved. Canvas is responsible for keeping any
        // one write()/finish() pair within that margin for its own unbounded inputs (see text()'s
        // per-chunk submission in canvas.cpp); a caller that instead feeds one shape with more points
        // than the margin allows (e.g. an extremely long beginShape()/vertex() path) will still hit the
        // appendVertex()/appendIndex() std::runtime_error below -- at that point it genuinely is a
        // misconfiguration to fix by raising initialMaxVertices/initialMaxIndices, not something
        // ordinary multi-shape drawing triggers. Must be followed by exactly one finish() call with the
        // same Writer, which submits whatever was written as a batch (merging into the previous batch
        // if blendMode/shader match).
        Writer write();
        void finish(const Writer& writer, const BlendMode& blendMode, const std::optional<rect2f>& clipRect, TextureFilter textureFilter, TextureWrap textureWrap, const Texture& texture, const Shader& shader);

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

        std::unique_ptr<Vertex[]> m_vertices;
        std::unique_ptr<uint32_t[]> m_indices;

        const size_t m_maxVertexCount;
        const size_t m_maxIndexCount;

        size_t m_currentVertexOffset;
        size_t m_currentIndexOffset;

        std::vector<RendererBatch> m_batches;
        matrix4x4 m_projectionMatrix;
        uint2 m_graphicsSize;
        Graphics m_graphics;
    };
} // namespace p5
