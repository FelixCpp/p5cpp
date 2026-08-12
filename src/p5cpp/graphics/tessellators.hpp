#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/vertex_sink.hpp>

#include <span>

namespace p5
{
    void tesselate_triangle(VertexSink& sink, const std::span<const float2, 3>& positions, const std::span<const float2, 3>& texCoords, const std::span<const float4, 3>& colors);
    void tesselate_triangles(VertexSink& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors);
    void tesselate_triangle_strip(VertexSink& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors);
    void tesselate_triangle_fan(VertexSink& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors);

    void tesselate_quad(VertexSink& sink, const std::span<const float2, 4>& positions, const std::span<const float2, 4>& texCoords, const std::span<const float4, 4>& colors);
    void tesselate_quads(VertexSink& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors);
    void tesselate_quad_strip(VertexSink& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors);

    void tesselate_polygon(VertexSink& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors);
} // namespace p5

namespace p5
{
    // `closed` treats `positions` as a loop (adds an implicit segment back from the last to the first point,
    // with a join instead of a cap at the seam) rather than an open polyline.
    void tesselate_path(VertexSink& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, bool closed = false);
}
