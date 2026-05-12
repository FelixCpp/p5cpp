#pragma once

#include "vertex.hpp"

#include <span>
// #include <memory>

namespace p5
{
    struct DrawScope
    {
        std::span<Vertex> vertices;
        std::span<uint32_t> indices;
        uint32_t& vertexCursor;
        uint32_t& indexCursor;
        uint32_t baseVertex;
        uint32_t baseIndex;

        uint32_t pushVertex(const float2& position, const float2& texcoord, const float4& color);
        void pushTriangle(uint32_t a, uint32_t b, uint32_t c);
    };
} // namespace p5
