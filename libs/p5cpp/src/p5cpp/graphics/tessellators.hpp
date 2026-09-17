#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/renderer.hpp>

#include <span>

namespace p5
{
    void tesselate_triangle(Renderer::Writer& sink, const std::span<const float2, 3>& positions, const std::span<const float2, 3>& texCoords, const std::span<const float4, 3>& colors);
    void tesselate_triangles(Renderer::Writer& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors);
    void tesselate_triangle_strip(Renderer::Writer& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors);
    void tesselate_triangle_fan(Renderer::Writer& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors);

    void tesselate_quad(Renderer::Writer& sink, const std::span<const float2, 4>& positions, const std::span<const float2, 4>& texCoords, const std::span<const float4, 4>& colors);
    void tesselate_quads(Renderer::Writer& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors);
    void tesselate_quad_strip(Renderer::Writer& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors);

    void tesselate_polygon(Renderer::Writer& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors);
} // namespace p5

namespace p5
{
    void tesselate_path(Renderer::Writer& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, bool closed = false, bool synthesizeCrossTrackV = false);
}
