#include "drawscope.hpp"

#include <cassert>

namespace p5
{
    uint32_t DrawScope::pushVertex(const float2& position, const float2& texcoord, const float4& color)
    {
        assert((vertexCursor < vertices.size()) && "CPU Vertex buffer overflow");

        vertices[vertexCursor++] = Vertex {
            .position = position,
            .texcoord = texcoord,
            .color = color,
            .texIndex = 0.0f, // Texture index will be assigned later by the renderer based on the current batch state
        };

        return vertexCursor - 1 - baseVertex;
    }

    void DrawScope::pushTriangle(uint32_t a, uint32_t b, uint32_t c)
    {
        assert((indexCursor + 3 <= indices.size()) && "CPU Index buffer overflow");

        indices[indexCursor++] = baseVertex + a;
        indices[indexCursor++] = baseVertex + b;
        indices[indexCursor++] = baseVertex + c;
    }
} // namespace p5
