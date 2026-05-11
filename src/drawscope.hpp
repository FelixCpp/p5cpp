#pragma once

#include "vertex.hpp"

#include <span>
// #include <memory>

namespace p5
{
    struct DrawScopeResult
    {
        size_t baseVertex;
        size_t baseIndex;
        size_t indexCount;
        size_t vertexCount;
    };

    class DrawScope
    {
    public:
        explicit DrawScope(std::span<Vertex> vertices, std::span<uint32_t> indices, uint32_t vertexCursor, uint32_t indexCursor);

        uint32_t pushVertex(const float2& position, const float2& texcoord, const float4& color);
        void pushTriangle(uint32_t a, uint32_t b, uint32_t c);

        DrawScopeResult build();

    private:
        std::span<Vertex> m_vertices;
        std::span<uint32_t> m_indices;
        uint32_t m_vertexCursor;
        uint32_t m_indexCursor;
        uint32_t m_baseVertex;
        uint32_t m_baseIndex;
    };
} // namespace p5
