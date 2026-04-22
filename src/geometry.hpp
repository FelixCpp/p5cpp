#pragma once

#include "p5.hpp"
#include "tess.hpp"

#include <span>

namespace p5
{
    struct Vertex
    {
        float3 position;
        float2 texcoord;
        float4 color;
    };

    struct GeometryPoint
    {
        float2 position;
        float2 texcoord;
        color_t fillColor;
        color_t strokeColor;
    };

    struct GeometryBuffer
    {
        std::unique_ptr<Vertex[]> vertices;
        size_t vertexCount;

        std::unique_ptr<uint32_t[]> indices;
        size_t indexCount;
    };

    struct GeometryBuilder
    {
        size_t initialVertexOffset;
        size_t initialIndexOffset;
        size_t vertexCount;
        size_t indexCount;

        GeometryBuffer* buffer;
        Tesselator* tesselator;

        void convex(const std::span<const GeometryPoint>& vertices);
        void concave(const std::span<const GeometryPoint>& vertices);
        void stroke(const std::span<const GeometryPoint>& vertices, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, StrokeAlign strokeAlign, bool close = true);

        void append(Vertex&& vertex);
        void addTriangle(uint32_t a, uint32_t b, uint32_t c);
    };
} // namespace p5
