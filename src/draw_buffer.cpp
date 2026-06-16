#include "draw_buffer.hpp"

namespace p5cpp
{
    DrawBuffer draw_buffer_create(size_t maxVertices, size_t maxIndices)
    {
        return DrawBuffer {
            .vertices = std::make_unique<Vertex[]>(maxVertices),
            .indices = std::make_unique<uint32_t[]>(maxIndices),
            .vertexCount = maxVertices,
            .indexCount = maxIndices,
            .vertexCursor = 0,
            .indexCursor = 0,
        };
    }

    DrawScope draw_buffer_get_scope(DrawBuffer& buffer)
    {
        return DrawScope {
            .vertices = std::span(buffer.vertices.get(), buffer.vertexCount),
            .indices = std::span(buffer.indices.get(), buffer.indexCount),
            .vertexCursor = buffer.vertexCursor,
            .indexCursor = buffer.indexCursor,
            .baseVertex = buffer.vertexCursor,
            .baseIndex = buffer.indexCursor,
        };
    }

    void draw_buffer_clear(DrawBuffer& buffer)
    {
        buffer.vertexCursor = 0;
        buffer.indexCursor = 0;
    }
} // namespace p5cpp
