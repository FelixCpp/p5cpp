#include <p5cpp/graphics/color.hpp>
#include <p5cpp/graphics/draw_buffer_writer.hpp>
#include <p5cpp/graphics/shaping.hpp>
#include <p5cpp/graphics/stroker.hpp>

#include <cassert>
#include <vector>
#include <tesselator.h>

namespace p5cpp
{
    inline static float4 color_to_float4(color_t color)
    {
        static constexpr float inv255 = 1.0f / 255.0f;

        return float4 {
            static_cast<float>(red(color)) * inv255,
            static_cast<float>(green(color)) * inv255,
            static_cast<float>(blue(color)) * inv255,
            static_cast<float>(alpha(color)) * inv255,
        };
    }
} // namespace p5cpp

namespace p5cpp
{
    thread_local std::vector<uint32_t> s_tess_local;

    // Reused across tesselate_polygon() calls to avoid allocating/freeing a full
    // TESStesselator (with internal mempools) on every polygon fill. libtess2 resets
    // tess->mesh to null on a successful tessTesselate(), so the handle is designed
    // to be reused via repeated tessAddContour()/tessTesselate() cycles.
    struct PooledTess
    {
        TESStesselator* tess = tessNewTess(nullptr);
        ~PooledTess() { tessDeleteTess(tess); }
    };
    thread_local PooledTess s_pooled_tess;
} // namespace p5cpp

namespace p5cpp
{
    void draw_scope_push_vertex(DrawBufferWriter& writer, const float2& position, const float2& texcoord, const float4& color)
    {
        writer.pushVertex(position, texcoord, color);
    }

    void draw_scope_push_triangle(DrawBufferWriter& writer, uint32_t a, uint32_t b, uint32_t c)
    {
        writer.pushTriangle(a, b, c);
    }
} // namespace p5cpp

namespace p5cpp
{
    void tesselate_quads(DrawBufferWriter& writer, const PathPoints& points)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 3 < n; i += 4) {
            // const size_t baseVertex = writer.vertexCursor - writer.baseVertex;
            const size_t baseVertex = writer.getRelativeCursor();
            draw_scope_push_vertex(writer, points.positions[i + 0], points.texcoords[i + 0], color_to_float4(points.colors[i + 0]));
            draw_scope_push_vertex(writer, points.positions[i + 1], points.texcoords[i + 1], color_to_float4(points.colors[i + 1]));
            draw_scope_push_vertex(writer, points.positions[i + 2], points.texcoords[i + 2], color_to_float4(points.colors[i + 2]));
            draw_scope_push_vertex(writer, points.positions[i + 3], points.texcoords[i + 3], color_to_float4(points.colors[i + 3]));

            draw_scope_push_triangle(writer, baseVertex + 0, baseVertex + 1, baseVertex + 2);
            draw_scope_push_triangle(writer, baseVertex + 0, baseVertex + 2, baseVertex + 3);
        }
    }

    void tesselate_quad_strip(DrawBufferWriter& writer, const PathPoints& points)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 3 < n; i += 2) {
            const size_t base = writer.getRelativeCursor();
            draw_scope_push_vertex(writer, points.positions[i + 0], points.texcoords[i + 0], color_to_float4(points.colors[i + 0]));
            draw_scope_push_vertex(writer, points.positions[i + 1], points.texcoords[i + 1], color_to_float4(points.colors[i + 1]));
            draw_scope_push_vertex(writer, points.positions[i + 2], points.texcoords[i + 2], color_to_float4(points.colors[i + 2]));
            draw_scope_push_vertex(writer, points.positions[i + 3], points.texcoords[i + 3], color_to_float4(points.colors[i + 3]));

            // Quad strip winding: (0,1,3) + (0,3,2)
            draw_scope_push_triangle(writer, base + 0, base + 1, base + 3);
            draw_scope_push_triangle(writer, base + 0, base + 3, base + 2);
        }
    }

    void tesselate_triangles(DrawBufferWriter& writer, const PathPoints& points)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 2 < n; i += 3) {
            const size_t baseVertex = writer.getRelativeCursor();
            draw_scope_push_vertex(writer, points.positions[i + 0], points.texcoords[i + 0], color_to_float4(points.colors[i + 0]));
            draw_scope_push_vertex(writer, points.positions[i + 1], points.texcoords[i + 1], color_to_float4(points.colors[i + 1]));
            draw_scope_push_vertex(writer, points.positions[i + 2], points.texcoords[i + 2], color_to_float4(points.colors[i + 2]));
            draw_scope_push_triangle(writer, baseVertex + 0, baseVertex + 1, baseVertex + 2);
        }
    }

    void tesselate_triangle_strip(DrawBufferWriter& writer, const PathPoints& points)
    {
        const size_t n = points.size;
        if (n < 3) return;

        const size_t base = writer.getRelativeCursor();
        for (size_t i = 0; i < n; ++i)
            draw_scope_push_vertex(writer, points.positions[i], points.texcoords[i], color_to_float4(points.colors[i]));

        for (size_t i = 0; i + 2 < n; ++i) {
            // Alternate winding every other triangle to keep consistent front-face.
            if ((i & 1) == 0)
                draw_scope_push_triangle(writer, base + i + 0, base + i + 1, base + i + 2);
            else
                draw_scope_push_triangle(writer, base + i + 1, base + i + 0, base + i + 2);
        }
    }

    void tesselate_triangle_fan(DrawBufferWriter& writer, const PathPoints& points)
    {
        const size_t n = points.size;
        if (n < 3) return;

        const size_t base = writer.getRelativeCursor();
        for (size_t i = 0; i < n; ++i)
            draw_scope_push_vertex(writer, points.positions[i], points.texcoords[i], color_to_float4(points.colors[i]));

        for (size_t i = 1; i + 1 < n; ++i)
            draw_scope_push_triangle(writer, base + 0, base + i, base + i + 1);
    }

    void tesselate_polygon(DrawBufferWriter& writer, const PathPoints& points)
    {
        const size_t n = points.size;
        if (n < 3) return;

        TESStesselator* tess = s_pooled_tess.tess;

        tessAddContour(tess, 2, points.positions.data(), static_cast<int>(sizeof(float2)), static_cast<int>(n));

        if (!tessTesselate(tess, TESS_WINDING_NONZERO, TESS_POLYGONS, 3, 2, nullptr)) {
            // tessTesselate() failed via its internal longjmp path without resetting
            // tess->mesh, so the pooled instance is left in a half-built state — fully
            // recycle it rather than risk corrupting the next call.
            tessDeleteTess(tess);
            s_pooled_tess.tess = tessNewTess(nullptr);
            return;
        }

        const int vertCount = tessGetVertexCount(tess);
        const float* verts = tessGetVertices(tess);
        const TESSindex* vertIdx = tessGetVertexIndices(tess);
        const int elemCount = tessGetElementCount(tess);
        const TESSindex* elems = tessGetElements(tess);

        s_tess_local.resize(static_cast<size_t>(vertCount));

        const size_t base = writer.getRelativeCursor();

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
            draw_scope_push_vertex(writer, pos, points.texcoords[src], color_to_float4(points.colors[src]));
            s_tess_local[v] = static_cast<uint32_t>(base + static_cast<size_t>(v));
        }

        for (int e = 0; e < elemCount; ++e) {
            TESSindex a = elems[e * 3 + 0];
            TESSindex b = elems[e * 3 + 1];
            TESSindex c = elems[e * 3 + 2];
            if (a == TESS_UNDEF || b == TESS_UNDEF || c == TESS_UNDEF) continue;
            draw_scope_push_triangle(writer, s_tess_local[a], s_tess_local[b], s_tess_local[c]);
        }
    }
} // namespace p5cpp

namespace p5cpp
{
    void stroke_quads(DrawBufferWriter& writer, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, const ComputeCircleSegmentCount& computeCircleSegmentCount)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 3 < n; i += 4) {
            float2 pos[4] = {points.positions[i], points.positions[i + 1], points.positions[i + 2], points.positions[i + 3]};
            float2 uvs[4] = {points.texcoords[i], points.texcoords[i + 1], points.texcoords[i + 2], points.texcoords[i + 3]};
            color_t cols[4] = {points.colors[i], points.colors[i + 1], points.colors[i + 2], points.colors[i + 3]};
            PathPoints sub = {4, pos, uvs, cols};
            generate_solid_stroke(writer, sub, strokeWeight, strokeCap, strokeJoin, miterLimit, roundJoinThreshold, true, computeCircleSegmentCount);
        }
    }

    void stroke_quad_strip(DrawBufferWriter& writer, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, const ComputeCircleSegmentCount& computeCircleSegmentCount)
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
            generate_solid_stroke(writer, sub, strokeWeight, strokeCap, strokeJoin, miterLimit, roundJoinThreshold, true, computeCircleSegmentCount);
        }
    }

    void stroke_triangles(DrawBufferWriter& writer, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, const ComputeCircleSegmentCount& computeCircleSegmentCount)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 2 < n; i += 3) {
            float2 pos[3] = {points.positions[i], points.positions[i + 1], points.positions[i + 2]};
            float2 uvs[3] = {points.texcoords[i], points.texcoords[i + 1], points.texcoords[i + 2]};
            color_t cols[3] = {points.colors[i], points.colors[i + 1], points.colors[i + 2]};
            PathPoints sub = {3, pos, uvs, cols};
            generate_solid_stroke(writer, sub, strokeWeight, strokeCap, strokeJoin, miterLimit, roundJoinThreshold, true, computeCircleSegmentCount);
        }
    }

    void stroke_triangle_strip(DrawBufferWriter& writer, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, const ComputeCircleSegmentCount& computeCircleSegmentCount)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 2 < n; ++i) {
            float2 pos[3] = {points.positions[i], points.positions[i + 1], points.positions[i + 2]};
            float2 uvs[3] = {points.texcoords[i], points.texcoords[i + 1], points.texcoords[i + 2]};
            color_t cols[3] = {points.colors[i], points.colors[i + 1], points.colors[i + 2]};
            PathPoints sub = {3, pos, uvs, cols};
            generate_solid_stroke(writer, sub, strokeWeight, strokeCap, strokeJoin, miterLimit, roundJoinThreshold, true, computeCircleSegmentCount);
        }
    }

    void stroke_triangle_fan(DrawBufferWriter& writer, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, const ComputeCircleSegmentCount& computeCircleSegmentCount)
    {
        const size_t n = points.size;
        if (n < 3) return;
        for (size_t i = 2; i < n; ++i) {
            float2 pos[3] = {points.positions[0], points.positions[i - 1], points.positions[i]};
            float2 uvs[3] = {points.texcoords[0], points.texcoords[i - 1], points.texcoords[i]};
            color_t cols[3] = {points.colors[0], points.colors[i - 1], points.colors[i]};
            PathPoints sub = {3, pos, uvs, cols};
            generate_solid_stroke(writer, sub, strokeWeight, strokeCap, strokeJoin, miterLimit, roundJoinThreshold, true, computeCircleSegmentCount);
        }
    }

    void stroke_lines(DrawBufferWriter& writer, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, float miterLimit, float roundJoinThreshold, const ComputeCircleSegmentCount& computeCircleSegmentCount)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 1 < n; i += 2) {
            float2 pos[2] = {points.positions[i], points.positions[i + 1]};
            float2 uvs[2] = {points.texcoords[i], points.texcoords[i + 1]};
            color_t cols[2] = {points.colors[i], points.colors[i + 1]};
            PathPoints sub = {2, pos, uvs, cols};
            generate_solid_stroke(writer, sub, strokeWeight, strokeCap, StrokeJoin::miter, miterLimit, roundJoinThreshold, false, computeCircleSegmentCount);
        }
    }

    void stroke_line_strip(DrawBufferWriter& writer, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, const ComputeCircleSegmentCount& computeCircleSegmentCount)
    {
        if (points.size < 2) return;
        generate_solid_stroke(writer, points, strokeWeight, strokeCap, strokeJoin, miterLimit, roundJoinThreshold, false, computeCircleSegmentCount);
    }

    void stroke_line_loop(DrawBufferWriter& writer, const PathPoints& points, float strokeWeight, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, const ComputeCircleSegmentCount& computeCircleSegmentCount)
    {
        if (points.size < 2) return;
        generate_solid_stroke(writer, points, strokeWeight, StrokeCap::butt, strokeJoin, miterLimit, roundJoinThreshold, true, computeCircleSegmentCount);
    }

    void stroke_polygon(DrawBufferWriter& writer, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, bool close, const ComputeCircleSegmentCount& computeCircleSegmentCount)
    {
        if (points.size < 2) return;
        generate_solid_stroke(writer, points, strokeWeight, strokeCap, strokeJoin, miterLimit, roundJoinThreshold, close, computeCircleSegmentCount);
    }
} // namespace p5cpp
