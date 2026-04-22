#include "geometry.hpp"

#include <cassert>

namespace p5
{
    static float4 color_to_float4(color_t color)
    {
        return float4 {
            .x = static_cast<float>(red(color)) / 255.0f,
            .y = static_cast<float>(green(color)) / 255.0f,
            .z = static_cast<float>(blue(color)) / 255.0f,
            .w = static_cast<float>(alpha(color)) / 255.0f,
        };
    }

    void GeometryBuilder::append(Vertex&& vertex)
    {
        assert((initialVertexOffset + vertexCount + 1 <= buffer->vertexCount) && "Exceeded vertex limit");

        buffer->vertices[initialVertexOffset + vertexCount] = std::move(vertex);
        vertexCount += 1;
    }

    void GeometryBuilder::addTriangle(uint32_t a, uint32_t b, uint32_t c)
    {
        assert((a < vertexCount) and (b < vertexCount) and (c < vertexCount) and "Exceeded index limit");

        buffer->indices[initialIndexOffset + indexCount + 0] = initialVertexOffset + a;
        buffer->indices[initialIndexOffset + indexCount + 1] = initialVertexOffset + b;
        buffer->indices[initialIndexOffset + indexCount + 2] = initialVertexOffset + c;

        indexCount += 3;
    }

    void GeometryBuilder::convex(const std::span<const GeometryPoint>& vertices)
    {
        for (size_t i = 0; i < vertices.size(); ++i) {
            const GeometryPoint& point = vertices[i];

            append(Vertex {
                .position = float3 {point.position.x, point.position.y, 0.0f},
                .texcoord = point.texcoord,
                .color = color_to_float4(point.fillColor),
            });
        }

        for (size_t i = 1; i < vertices.size() - 1; ++i) {
            addTriangle(0, i, i + 1);
        }
    }

    void GeometryBuilder::concave(const std::span<const GeometryPoint>& vertices)
    {
    }

    void GeometryBuilder::stroke(const std::span<const GeometryPoint>& vertices, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, StrokeAlign strokeAlign, bool close)
    {
    }
} // namespace p5
