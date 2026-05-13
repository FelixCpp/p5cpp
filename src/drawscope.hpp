#pragma once

#include "vertex.hpp"

#include <span>
#include <memory>

namespace p5
{
    struct DrawBuffer
    {
        std::unique_ptr<Vertex[]> vertices;
        std::unique_ptr<uint32_t[]> indices;

        size_t vertexCount;
        size_t indexCount;

        size_t vertexCursor;
        size_t indexCursor;
    };

    struct DrawScope
    {
        std::span<Vertex> vertices;
        std::span<uint32_t> indices;
        size_t& vertexCursor;
        size_t& indexCursor;
        size_t baseVertex;
        size_t baseIndex;
    };

    DrawBuffer draw_buffer_create(size_t maxVertices, size_t maxIndices);
    DrawScope draw_buffer_get_scope(DrawBuffer& buffer);
    void draw_buffer_clear(DrawBuffer& buffer);

    void draw_scope_push_vertex(DrawScope& scope, const float2& position, const float2& texcoord, const float4& color);
    void draw_scope_push_triangle(DrawScope& scope, uint32_t a, uint32_t b, uint32_t c);
} // namespace p5
