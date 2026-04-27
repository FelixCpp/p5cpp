#include "drawscope.hpp"

#include <span>
#include <cassert>

namespace p5
{
    DrawScope::DrawScope(const std::span<Vertex>& vertices, const std::span<uint32_t>& indices, size_t vertexStart, size_t indexStart)
        : vertices(vertices), indices(indices), vertexStart(vertexStart), vertexCount(0), indexStart(indexStart), indexCount(0)
    {
    }

    size_t DrawScope::getVertexCount() const
    {
        return vertexCount;
    }

    size_t DrawScope::getIndexCount() const
    {
        return indexCount;
    }

    void DrawScope::push(Vertex&& vertex)
    {
        assert(vertexStart + vertexCount + 1 < vertices.size() && "Vertices overflow");
        vertices[vertexStart + vertexCount++] = std::move(vertex);
    }

    void DrawScope::push(uint32_t index)
    {
        assert(vertexStart + vertexCount + 1 < vertices.size() && "Indices overflow");
        indices[indexStart + indexCount++] = vertexStart + index;
    }
} // namespace p5
