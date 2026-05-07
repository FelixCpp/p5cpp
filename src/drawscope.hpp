#pragma once

#include "vertex.hpp"

#include <span>
// #include <memory>

namespace p5
{
    // class CPUDrawBufferWriter
    // {
    // public:
    //     void pushVertex(const float2& position, const float2& texcoord, const float4& color);
    //     void pushTriangle(uint32_t a, uint32_t b, uint32_t c);
    //     void setTextureIndex(float textureIndex);
    //
    // private:
    //     std::span<Vertex> m_vertices;  // Write-View on the original vertex buffer
    //     std::span<uint32_t> m_indices; // Write-View on the original index buffer
    //
    //     size_t baseVertex; // Offset in the vertex buffer where the current draw scope starts
    //     size_t baseIndex;  // Offset in the index buffer where the current draw scope starts
    // };
    //
    // class CPUDrawBuffer
    // {
    // public:
    //     explicit CPUDrawBuffer(size_t maxVertices, size_t maxIndices);
    //
    //     CPUDrawBufferWriter aquireWriter();
    //
    // private:
    //     std::unique_ptr<Vertex[]> m_vertices;
    //     std::unique_ptr<uint32_t[]> m_indices;
    //
    //     size_t m_maxVertices;
    //     size_t m_maxIndices;
    //
    //     size_t m_vertexCursor;
    //     size_t m_indexCursor;
    // };

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
