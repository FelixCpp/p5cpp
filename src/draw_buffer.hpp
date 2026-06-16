#pragma once

#include "draw_scope.hpp"
#include "vertex.hpp"

namespace p5cpp
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

    DrawBuffer draw_buffer_create(size_t maxVertices, size_t maxIndices);
    DrawScope draw_buffer_get_scope(DrawBuffer& buffer);
    void draw_buffer_clear(DrawBuffer& buffer);
} // namespace p5cpp
