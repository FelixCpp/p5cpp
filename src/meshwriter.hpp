#pragma once

#include "vertex.hpp"

#include <span>

namespace p5
{

    class MeshWriter
    {
    public:
        explicit MeshWriter(std::span<Vertex> vertices, std::span<uint32_t> indices, uint32_t& vertexCursor, uint32_t& indexCursor);

        void pushVertex(const float2& position, const float2& texcoord, const float4& color);
        void pushTriangle(uint32_t a, uint32_t b, uint32_t c);
        void setTextureIndex(float textureIndex);

        uint32_t getBaseVertex() const;
        uint32_t getVertexCount() const;
        uint32_t getBaseIndex() const;
        uint32_t getIndexCount() const;

    private:
        std::span<Vertex> m_vertices;
        std::span<uint32_t> m_indices;
        uint32_t& m_vertexCursor;
        uint32_t& m_indexCursor;
        uint32_t m_baseVertex;
        uint32_t m_baseIndex;
    };

} // namespace p5
