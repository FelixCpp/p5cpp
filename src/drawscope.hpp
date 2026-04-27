#pragma once

#include <span>

#include "vertex.hpp"

namespace p5
{
    class DrawScope
    {
    public:
        explicit DrawScope(const std::span<Vertex>& vertices, const std::span<uint32_t>& indices, size_t vertexStart, size_t indexStart);

        size_t getVertexCount() const;
        size_t getIndexCount() const;

        void push(Vertex&& vertex);
        void push(uint32_t index);

    private:
        std::span<Vertex> vertices;
        std::span<uint32_t> indices;

        size_t vertexStart;
        size_t vertexCount;

        size_t indexStart;
        size_t indexCount;
    };
} // namespace p5
