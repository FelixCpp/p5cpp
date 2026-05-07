#include "drawscope.hpp"

#include <cassert>

namespace p5
{
    DrawScope::DrawScope(std::span<Vertex> vertices, std::span<uint32_t> indices, uint32_t vertexCursor, uint32_t indexCursor)
        : m_vertices(vertices), m_indices(indices), m_vertexCursor(vertexCursor), m_indexCursor(indexCursor), m_baseVertex(vertexCursor), m_baseIndex(indexCursor)
    {
    }

    uint32_t DrawScope::pushVertex(const float2& position, const float2& texcoord, const float4& color)
    {
        assert((m_vertexCursor < m_vertices.size()) && "CPU Vertex buffer overflow");

        m_vertices[m_vertexCursor++] = Vertex {
            .position = position,
            .texcoord = texcoord,
            .color = color,
            .texIndex = 0.0f, // Texture index will be assigned later by the renderer based on the current batch state
        };

        return m_vertexCursor - 1 - m_baseVertex;
    }

    void DrawScope::pushTriangle(uint32_t a, uint32_t b, uint32_t c)
    {
        assert((m_indexCursor + 3 <= m_indices.size()) && "CPU Index buffer overflow");

        m_indices[m_indexCursor++] = m_baseVertex + a;
        m_indices[m_indexCursor++] = m_baseVertex + b;
        m_indices[m_indexCursor++] = m_baseVertex + c;
    }

    DrawScopeResult DrawScope::build()
    {
        return DrawScopeResult {
            .baseVertex = m_baseVertex,
            .baseIndex = m_baseIndex,
            .indexCount = m_indexCursor - m_baseIndex,
            .vertexCount = m_vertexCursor - m_baseVertex,
        };
    }

} // namespace p5
