#include "tess.hpp"

namespace p5
{
    inline static float4 color_to_float4(color_t color)
    {
        return float4 {
            .x = static_cast<float>(red(color)) / 255.0f,
            .y = static_cast<float>(green(color)) / 255.0f,
            .z = static_cast<float>(blue(color)) / 255.0f,
            .w = static_cast<float>(alpha(color)) / 255.0f,
        };
    }
} // namespace p5

namespace p5
{
    void tesselate_quads(DrawScope& scope, const PathPoints& points)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 3 < n; i += 4) {
            const size_t baseVertex = scope.vertexCursor - scope.baseVertex;
            draw_scope_push_vertex(scope, points.positions[i + 0], points.texcoords[i + 0], color_to_float4(points.colors[i + 0]));
            draw_scope_push_vertex(scope, points.positions[i + 1], points.texcoords[i + 1], color_to_float4(points.colors[i + 1]));
            draw_scope_push_vertex(scope, points.positions[i + 2], points.texcoords[i + 2], color_to_float4(points.colors[i + 2]));
            draw_scope_push_vertex(scope, points.positions[i + 3], points.texcoords[i + 3], color_to_float4(points.colors[i + 3]));

            draw_scope_push_triangle(scope, baseVertex + 0, baseVertex + 1, baseVertex + 2);
            draw_scope_push_triangle(scope, baseVertex + 0, baseVertex + 2, baseVertex + 3);
        }
    }

    void tesselate_quad_strip(DrawScope& scope, const PathPoints& points)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 3 < n; i += 2) {
            const size_t baseVertex = scope.vertexCursor - scope.baseVertex;
            draw_scope_push_vertex(scope, points.positions[i + 0], points.texcoords[i + 0], color_to_float4(points.colors[i + 0]));
            draw_scope_push_vertex(scope, points.positions[i + 1], points.texcoords[i + 1], color_to_float4(points.colors[i + 1]));
            draw_scope_push_vertex(scope, points.positions[i + 2], points.texcoords[i + 2], color_to_float4(points.colors[i + 2]));
            draw_scope_push_vertex(scope, points.positions[i + 3], points.texcoords[i + 3], color_to_float4(points.colors[i + 3]));

            draw_scope_push_triangle(scope, baseVertex + 0, baseVertex + 1, baseVertex + 2);
            draw_scope_push_triangle(scope, baseVertex + 0, baseVertex + 2, baseVertex + 3);
        }
    }

    void tesselate_triangles(DrawScope& scope, const PathPoints& points)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 2 < n; i += 3) {
            const size_t baseVertex = scope.vertexCursor - scope.baseVertex;
            draw_scope_push_vertex(scope, points.positions[i + 0], points.texcoords[i + 0], color_to_float4(points.colors[i + 0]));
            draw_scope_push_vertex(scope, points.positions[i + 1], points.texcoords[i + 1], color_to_float4(points.colors[i + 1]));
            draw_scope_push_vertex(scope, points.positions[i + 2], points.texcoords[i + 2], color_to_float4(points.colors[i + 2]));
            draw_scope_push_triangle(scope, baseVertex + 0, baseVertex + 1, baseVertex + 2);
        }
    }

    void tesselate_triangle_strip(DrawScope& scope, const PathPoints& points)
    {
        const size_t n = points.size;
        if (n < 3) return;

        uint32_t a = push_pt(scope, points, 0);
        uint32_t b = push_pt(scope, points, 1);
        for (size_t i = 2; i < n; ++i) {
            uint32_t c = push_pt(scope, points, i);
            if ((i & 1) == 0)
                draw_scope_push_triangle(scope, a, b, c);
            else
                draw_scope_push_triangle(scope, b, a, c);
            a = b;
            b = c;
        }
    }

    void tesselate_triangle_fan(DrawScope& scope, const PathPoints& points)
    {
    }

    void tesselate_polygon(DrawScope& scope, const PathPoints& points)
    {
    }
} // namespace p5

namespace p5
{
    void stroke_quads(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold)
    {
    }

    void stroke_quad_strip(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold)
    {
    }

    void stroke_triangles(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold)
    {
    }

    void stroke_triangle_strip(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold)
    {
    }

    void stroke_triangle_fan(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold)
    {
    }

    void stroke_lines(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, float miterLimit, float roundJoinThreshold)
    {
    }

    void stroke_line_strip(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold)
    {
    }

    void stroke_line_loop(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold)
    {
    }

    void stroke_polygon(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold)
    {
    }
} // namespace p5
