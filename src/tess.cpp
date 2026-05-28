#include "tess.hpp"
#include "stroker.hpp"
#include <cassert>
#include <tesselator.h>

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
    thread_local std::vector<uint32_t> s_tess_local;
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
            const size_t base = scope.vertexCursor - scope.baseVertex;
            draw_scope_push_vertex(scope, points.positions[i + 0], points.texcoords[i + 0], color_to_float4(points.colors[i + 0]));
            draw_scope_push_vertex(scope, points.positions[i + 1], points.texcoords[i + 1], color_to_float4(points.colors[i + 1]));
            draw_scope_push_vertex(scope, points.positions[i + 2], points.texcoords[i + 2], color_to_float4(points.colors[i + 2]));
            draw_scope_push_vertex(scope, points.positions[i + 3], points.texcoords[i + 3], color_to_float4(points.colors[i + 3]));

            // Quad strip winding: (0,1,3) + (0,3,2)
            draw_scope_push_triangle(scope, base + 0, base + 1, base + 3);
            draw_scope_push_triangle(scope, base + 0, base + 3, base + 2);
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

        const size_t base = scope.vertexCursor - scope.baseVertex;
        for (size_t i = 0; i < n; ++i)
            draw_scope_push_vertex(scope, points.positions[i], points.texcoords[i], color_to_float4(points.colors[i]));

        for (size_t i = 0; i + 2 < n; ++i) {
            // Alternate winding every other triangle to keep consistent front-face.
            if ((i & 1) == 0)
                draw_scope_push_triangle(scope, base + i + 0, base + i + 1, base + i + 2);
            else
                draw_scope_push_triangle(scope, base + i + 1, base + i + 0, base + i + 2);
        }
    }

    void tesselate_triangle_fan(DrawScope& scope, const PathPoints& points)
    {
        const size_t n = points.size;
        if (n < 3) return;

        const size_t base = scope.vertexCursor - scope.baseVertex;
        for (size_t i = 0; i < n; ++i)
            draw_scope_push_vertex(scope, points.positions[i], points.texcoords[i], color_to_float4(points.colors[i]));

        for (size_t i = 1; i + 1 < n; ++i)
            draw_scope_push_triangle(scope, base + 0, base + i, base + i + 1);
    }

    void tesselate_polygon(DrawScope& scope, const PathPoints& points)
    {
        const size_t n = points.size;
        if (n < 3) return;

        TESStesselator* tess = tessNewTess(nullptr);

        tessAddContour(tess, 2, points.positions.data(), static_cast<int>(sizeof(float2)), static_cast<int>(n));

        if (!tessTesselate(tess, TESS_WINDING_NONZERO, TESS_POLYGONS, 3, 2, nullptr)) {
            tessDeleteTess(tess);
            return;
        }

        const int vertCount = tessGetVertexCount(tess);
        const float* verts = tessGetVertices(tess);
        const TESSindex* vertIdx = tessGetVertexIndices(tess);
        const int elemCount = tessGetElementCount(tess);
        const TESSindex* elems = tessGetElements(tess);

        s_tess_local.resize(static_cast<size_t>(vertCount));

        const size_t base = scope.vertexCursor - scope.baseVertex;

        size_t lastSrc = 0;
        for (int v = 0; v < vertCount; ++v) {
            size_t src;
            if (vertIdx[v] != TESS_UNDEF) {
                src = static_cast<size_t>(vertIdx[v]);
                lastSrc = src;
            } else {
                src = lastSrc;
            }
            float2 pos = {verts[v * 2 + 0], verts[v * 2 + 1]};
            draw_scope_push_vertex(scope, pos, points.texcoords[src], color_to_float4(points.colors[src]));
            s_tess_local[v] = static_cast<uint32_t>(base + static_cast<size_t>(v));
        }

        for (int e = 0; e < elemCount; ++e) {
            TESSindex a = elems[e * 3 + 0];
            TESSindex b = elems[e * 3 + 1];
            TESSindex c = elems[e * 3 + 2];
            if (a == TESS_UNDEF || b == TESS_UNDEF || c == TESS_UNDEF) continue;
            draw_scope_push_triangle(scope, s_tess_local[a], s_tess_local[b], s_tess_local[c]);
        }

        tessDeleteTess(tess);
    }
} // namespace p5

namespace p5
{
    void stroke_quads(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 3 < n; i += 4) {
            float2 pos[4] = {points.positions[i], points.positions[i + 1], points.positions[i + 2], points.positions[i + 3]};
            float2 uvs[4] = {points.texcoords[i], points.texcoords[i + 1], points.texcoords[i + 2], points.texcoords[i + 3]};
            color_t cols[4] = {points.colors[i], points.colors[i + 1], points.colors[i + 2], points.colors[i + 3]};
            PathPoints sub = {4, pos, uvs, cols};
            generate_solid_stroke(scope, sub, strokeWeight, strokeCap, strokeJoin, miterLimit, roundJoinThreshold, true);
        }
    }

    void stroke_quad_strip(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 3 < n; i += 2) {
            // Layout:  i+0 --- i+2
            //          |           |
            //          i+1 --- i+3
            // CCW outline: i+0 → i+2 → i+3 → i+1
            float2 pos[4] = {points.positions[i], points.positions[i + 2], points.positions[i + 3], points.positions[i + 1]};
            float2 uvs[4] = {points.texcoords[i], points.texcoords[i + 2], points.texcoords[i + 3], points.texcoords[i + 1]};
            color_t cols[4] = {points.colors[i], points.colors[i + 2], points.colors[i + 3], points.colors[i + 1]};
            PathPoints sub = {4, pos, uvs, cols};
            generate_solid_stroke(scope, sub, strokeWeight, strokeCap, strokeJoin, miterLimit, roundJoinThreshold, true);
        }
    }

    void stroke_triangles(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 2 < n; i += 3) {
            float2 pos[3] = {points.positions[i], points.positions[i + 1], points.positions[i + 2]};
            float2 uvs[3] = {points.texcoords[i], points.texcoords[i + 1], points.texcoords[i + 2]};
            color_t cols[3] = {points.colors[i], points.colors[i + 1], points.colors[i + 2]};
            PathPoints sub = {3, pos, uvs, cols};
            generate_solid_stroke(scope, sub, strokeWeight, strokeCap, strokeJoin, miterLimit, roundJoinThreshold, true);
        }
    }

    void stroke_triangle_strip(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 2 < n; ++i) {
            float2 pos[3] = {points.positions[i], points.positions[i + 1], points.positions[i + 2]};
            float2 uvs[3] = {points.texcoords[i], points.texcoords[i + 1], points.texcoords[i + 2]};
            color_t cols[3] = {points.colors[i], points.colors[i + 1], points.colors[i + 2]};
            PathPoints sub = {3, pos, uvs, cols};
            generate_solid_stroke(scope, sub, strokeWeight, strokeCap, strokeJoin, miterLimit, roundJoinThreshold, true);
        }
    }

    void stroke_triangle_fan(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold)
    {
        const size_t n = points.size;
        if (n < 3) return;
        for (size_t i = 2; i < n; ++i) {
            float2 pos[3] = {points.positions[0], points.positions[i - 1], points.positions[i]};
            float2 uvs[3] = {points.texcoords[0], points.texcoords[i - 1], points.texcoords[i]};
            color_t cols[3] = {points.colors[0], points.colors[i - 1], points.colors[i]};
            PathPoints sub = {3, pos, uvs, cols};
            generate_solid_stroke(scope, sub, strokeWeight, strokeCap, strokeJoin, miterLimit, roundJoinThreshold, true);
        }
    }

    void stroke_lines(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, float miterLimit, float roundJoinThreshold)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 1 < n; i += 2) {
            float2 pos[2] = {points.positions[i], points.positions[i + 1]};
            float2 uvs[2] = {points.texcoords[i], points.texcoords[i + 1]};
            color_t cols[2] = {points.colors[i], points.colors[i + 1]};
            PathPoints sub = {2, pos, uvs, cols};
            generate_solid_stroke(scope, sub, strokeWeight, strokeCap, StrokeJoin::miter, miterLimit, roundJoinThreshold, false);
        }
    }

    void stroke_line_strip(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold)
    {
        if (points.size < 2) return;
        generate_solid_stroke(scope, points, strokeWeight, strokeCap, strokeJoin, miterLimit, roundJoinThreshold, false);
    }

    void stroke_line_loop(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold)
    {
        if (points.size < 2) return;
        generate_solid_stroke(scope, points, strokeWeight, StrokeCap::butt, strokeJoin, miterLimit, roundJoinThreshold, true);
    }

    void stroke_polygon(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, bool close)
    {
        if (points.size < 2) return;
        generate_solid_stroke(scope, points, strokeWeight, strokeCap, strokeJoin, miterLimit, roundJoinThreshold, close);
    }
} // namespace p5
