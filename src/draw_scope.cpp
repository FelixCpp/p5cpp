#include "draw_scope.hpp"

#include <cassert>

namespace p5cpp
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
} // namespace p5cpp
