#include "meshwriter.hpp"

#include <cassert>

namespace p5
{

    MeshWriter::MeshWriter(std::span<Vertex> vertices, std::span<uint32_t> indices, uint32_t& vertexCursor, uint32_t& indexCursor)
        : m_vertices(vertices), m_indices(indices), m_vertexCursor(vertexCursor), m_indexCursor(indexCursor), m_baseVertex(vertexCursor), m_baseIndex(indexCursor)
    {
    }

    void MeshWriter::pushVertex(const float2& position, const float2& texcoord, const float4& color)
    {
        assert((m_vertexCursor < m_vertices.size()) && "CPU Vertex buffer overflow");

        m_vertices[m_vertexCursor++] = Vertex {
            .position = position,
            .texcoord = texcoord,
            .color = color,
            .texIndex = 0.0f, // Texture index will be assigned later by the renderer based on the current batch state
        };
    }

    void MeshWriter::pushTriangle(uint32_t a, uint32_t b, uint32_t c)
    {
        assert((m_indexCursor + 3 <= m_indices.size()) && "CPU Index buffer overflow");

        m_indices[m_indexCursor++] = m_baseVertex + a;
        m_indices[m_indexCursor++] = m_baseVertex + b;
        m_indices[m_indexCursor++] = m_baseVertex + c;
    }

    uint32_t MeshWriter::getBaseVertex() const
    {
        return m_baseVertex;
    }

    uint32_t MeshWriter::getVertexCount() const
    {
        return m_vertexCursor - m_baseVertex;
    }

    uint32_t MeshWriter::getBaseIndex() const
    {
        return m_baseIndex;
    }

    uint32_t MeshWriter::getIndexCount() const
    {
        return m_indexCursor - m_baseIndex;
    }

} // namespace p5
