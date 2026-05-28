#include "drawscope.hpp"

#include <cassert>

namespace p5
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
} // namespace p5

namespace p5
{
    void draw_scope_push_vertex(DrawScope& scope, const float2& position, const float2& texcoord, const float4& color)
    {
        if (scope.vertexCursor >= scope.vertices.size()) {
            // Buffer is full; the caller should have called flushIfNeeded() before
            // creating this scope.  Silently drop the vertex to prevent an OOB write.
            return;
        }

        scope.vertices[scope.vertexCursor++] = Vertex {
            .position = position,
            .texcoord = texcoord,
            .color = color,
            .texIndex = 0.0f, // Texture index will be assigned later by the renderer based on the current batch state
        };
    }

    void draw_scope_push_triangle(DrawScope& scope, uint32_t a, uint32_t b, uint32_t c)
    {
        if (scope.indexCursor + 2 >= scope.indices.size()) {
            // Buffer is full; silently drop to prevent an OOB write.
            return;
        }

        scope.indices[scope.indexCursor++] = scope.baseVertex + a;
        scope.indices[scope.indexCursor++] = scope.baseVertex + b;
        scope.indices[scope.indexCursor++] = scope.baseVertex + c;
    }
} // namespace p5
