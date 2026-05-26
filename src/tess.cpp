#include "tess.hpp"
#include "linepath.hpp"
#include "drawscope.hpp"
#include "stroker.hpp"

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

// ===========================================================================
// Internal helpers
// ===========================================================================

namespace p5
{
    // Shorthand accessors – no bounds-check needed, caller guarantees validity.
    inline float2 pt(const p5::PathPoints& p, size_t i) { return p.positions[i]; }
    inline float2 uv(const p5::PathPoints& p, size_t i) { return p.texcoords[i]; }
    inline color_t col(const p5::PathPoints& p, size_t i) { return p.colors[i]; }

    // Bilinear interpolation helpers for caps/line-fill interpolation.
    inline float2 lerp2(float2 a, float2 b, float t)
    {
        return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
    }

    inline color_t lerp_col(color_t a, color_t b, float t)
    {
        // Assumes color_t has r,g,b,a floats – adjust as needed.
        return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
    }

    // Push one vertex from PathPoints and return its local draw-scope index.
    inline uint32_t push_pt(DrawScope& scope, const p5::PathPoints& pts, size_t i)
    {
        draw_scope_push_vertex(scope, pts.positions[i], pts.texcoords[i], color_to_float4(pts.colors[i]));
        return static_cast<uint32_t>((scope.vertexCursor - 1) - scope.baseVertex);
    }

    // Push an ad-hoc vertex (position/uv/color already computed).
    inline uint32_t push_raw(DrawScope& scope, float2 pos, float2 uv_, color_t c)
    {
        draw_scope_push_vertex(scope, pos, uv_, color_to_float4(c));
        return static_cast<uint32_t>((scope.vertexCursor - 1) - scope.baseVertex);
    }

    // ---------------------------------------------------------------------------
    // libtess2 polygon fill helper
    //   Contour must already be wound CCW for filled exterior.
    //   Interpolates uv/color from the original PathPoints by nearest vertex.
    // ---------------------------------------------------------------------------
    void tess_fill(DrawScope& scope, const p5::PathPoints& points)
    {
        const size_t n = points.size;
        if (n < 3) return;

        TESStesselator* tess = tessNewTess(nullptr);

        // Feed the single contour.  libtess2 wants float[2] pairs.
        // positions is contiguous float2 → cast directly.
        tessAddContour(tess, 2, points.positions.data(), sizeof(float2), static_cast<int>(n));

        if (!tessTesselate(tess, TESS_WINDING_NONZERO, TESS_POLYGONS, 3, 2, nullptr)) {
            tessDeleteTess(tess);
            return;
        }

        const int vert_count = tessGetVertexCount(tess);
        const float* verts = tessGetVertices(tess);
        const int elem_count = tessGetElementCount(tess);
        const TESSindex* elems = tessGetElements(tess);

        // For each tessellated vertex we look up the nearest source vertex to
        // carry along its uv and color.  For many shapes the tessellator returns
        // the original verts verbatim, but new Steiner points may be introduced.
        auto nearest_src = [&](float x, float y) -> size_t {
            size_t best = 0;
            float best_d2 = 1e30f;
            for (size_t i = 0; i < n; ++i) {
                float dx = points.positions[i].x - x;
                float dy = points.positions[i].y - y;
                float d2 = dx * dx + dy * dy;
                if (d2 < best_d2) {
                    best_d2 = d2;
                    best = i;
                }
            }
            return best;
        };

        // Push tessellated vertices and build a local-index map.
        std::vector<uint32_t> local(static_cast<size_t>(vert_count));
        for (int v = 0; v < vert_count; ++v) {
            float x = verts[v * 2 + 0];
            float y = verts[v * 2 + 1];
            size_t src = nearest_src(x, y);
            local[v] = push_raw(scope, {x, y}, points.texcoords[src], points.colors[src]);
        }

        // Push triangles.
        for (int e = 0; e < elem_count; ++e) {
            TESSindex a = elems[e * 3 + 0];
            TESSindex b = elems[e * 3 + 1];
            TESSindex c = elems[e * 3 + 2];
            if (a == TESS_UNDEF || b == TESS_UNDEF || c == TESS_UNDEF) continue;
            draw_scope_push_triangle(scope, local[a], local[b], local[c]);
        }

        tessDeleteTess(tess);
    }

    // ---------------------------------------------------------------------------
    // Line-segment fill (rectangle + optional caps)
    //   Generates a quad for the body and cap geometry for each endpoint.
    //   uv/color are interpolated linearly along the segment (t=0 → t=1).
    // ---------------------------------------------------------------------------
    void fill_segment(DrawScope& scope, float2 p0, float2 p1, float2 uv0, float2 uv1, color_t c0, color_t c1, float halfW, StrokeCap cap)
    {
        float2 dir = {p1.x - p0.x, p1.y - p0.y};
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len < 1e-6f) return;

        float2 unit = {dir.x / len, dir.y / len};
        float2 perp = {-unit.y * halfW, unit.x * halfW};

        // Body quad: 4 corners of the rectangle.
        //   tl --- tr
        //   |       |
        //   bl --- br
        float2 tl = {p0.x + perp.x, p0.y + perp.y};
        float2 bl = {p0.x - perp.x, p0.y - perp.y};
        float2 tr = {p1.x + perp.x, p1.y + perp.y};
        float2 br = {p1.x - perp.x, p1.y - perp.y};

        // Square cap: extend body outward by halfW along the direction.
        if (cap.start == StrokeCapStyle::square) {
            float2 ext0 = {-unit.x * halfW, -unit.y * halfW};
            float2 ext1 = {unit.x * halfW, unit.y * halfW};
            tl.x += ext0.x;
            tl.y += ext0.y;
            bl.x += ext0.x;
            bl.y += ext0.y;
            tr.x += ext1.x;
            tr.y += ext1.y;
            br.x += ext1.x;
            br.y += ext1.y;
        }

        uint32_t iTL = push_raw(scope, tl, uv0, c0);
        uint32_t iBL = push_raw(scope, bl, uv0, c0);
        uint32_t iTR = push_raw(scope, tr, uv1, c1);
        uint32_t iBR = push_raw(scope, br, uv1, c1);

        draw_scope_push_triangle(scope, iTL, iBL, iBR);
        draw_scope_push_triangle(scope, iTL, iBR, iTR);

        // Round cap: semicircle of triangles fanned from each endpoint.
        if (cap.start == StrokeCapStyle::round) {
            constexpr int kRoundSteps = 8; // half-circle subdivisions
            constexpr float kPi = 3.14159265358979f;

            auto fan_cap = [&](float2 center, float2 base_perp,
                               float2 forward_dir, // points AWAY from segment
                               float2 uvc,
                               color_t cc,
                               uint32_t edge_a,
                               uint32_t edge_b) {
                // edge_a and edge_b are the two body corners at this endpoint.
                // We fan from center, sweeping 180° in the outward hemisphere.
                uint32_t hub = push_raw(scope, center, uvc, cc);

                // Start angle: direction of edge_a relative to center = +perp
                // We sweep from +perp to -perp through the outward half.
                // Angle of +perp from positive-X:
                float angle_start = std::atan2(base_perp.y, base_perp.x);
                float angle_end = std::atan2(-base_perp.y, -base_perp.x);

                // Ensure we sweep through the outward side.
                // The outward direction angle:
                float angle_out = std::atan2(forward_dir.y, forward_dir.x);
                // Normalise angle_end to be on the same side as angle_out.
                while (angle_end - angle_start > kPi) angle_end -= 2.f * kPi;
                while (angle_end - angle_start < -kPi) angle_end += 2.f * kPi;
                // If the midpoint of our sweep is opposite to forward_dir, flip.
                float mid = (angle_start + angle_end) * 0.5f;
                float da = angle_out - mid;
                while (da > kPi) da -= 2.f * kPi;
                while (da < -kPi) da += 2.f * kPi;
                if (std::abs(da) > kPi * 0.5f)
                    std::swap(angle_start, angle_end);

                uint32_t prev_rim = edge_a;
                for (int s = 1; s <= kRoundSteps; ++s) {
                    float t = static_cast<float>(s) / static_cast<float>(kRoundSteps);
                    float angle = angle_start + t * (angle_end - angle_start);
                    float2 rim = {center.x + std::cos(angle) * halfW, center.y + std::sin(angle) * halfW};
                    uint32_t cur_rim = (s == kRoundSteps)
                                           ? edge_b
                                           : push_raw(scope, rim, uvc, cc);
                    draw_scope_push_triangle(scope, hub, prev_rim, cur_rim);
                    prev_rim = cur_rim;
                }
            };

            // p0 cap: outward direction is -unit
            float2 neg_unit = {-unit.x, -unit.y};
            fan_cap(p0, perp, neg_unit, uv0, c0, iTL, iBL);

            // p1 cap: outward direction is +unit
            fan_cap(p1, {-perp.x, -perp.y}, unit, uv1, c1, iBR, iTR);
        }
    }

    // Build a sub-PathPoints on the stack from a fixed-size local array.
    // Avoids heap allocation for small primitives.
    template <size_t N>
    struct SubPoints
    {
        float2 pos[N];
        float2 uvs[N];
        color_t cols[N];

        p5::PathPoints make() const
        {
            return p5::PathPoints {
                .size = N,
                .positions = pos,
            };
            p5::PathPoints p;
            p.size = count;
            p.positions = std::span<const float2> {pos, count};
            p.texcoords = std::span<const float2> {uvs, count};
            p.colors = std::span<const color_t> {cols, count};
            return p;
        }

        void set(size_t i, const p5::PathPoints& src, size_t si)
        {
            pos[i] = src.positions[si];
            uvs[i] = src.texcoords[si];
            cols[i] = src.colors[si];
        }
    };

} // namespace p5

// ===========================================================================
// TESSELLATION  (fill)
// ===========================================================================

namespace p5
{

    // ---------------------------------------------------------------------------
    // QUADS  – every 4 vertices → independent quad (2 triangles)
    // ---------------------------------------------------------------------------
    void tesselate_quads(DrawScope& scope, const PathPoints& points)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 3 < n; i += 4) {
            uint32_t a = push_pt(scope, points, i + 0);
            uint32_t b = push_pt(scope, points, i + 1);
            uint32_t c = push_pt(scope, points, i + 2);
            uint32_t d = push_pt(scope, points, i + 3);
            draw_scope_push_triangle(scope, a, b, c);
            draw_scope_push_triangle(scope, a, c, d);
        }
    }

    // ---------------------------------------------------------------------------
    // QUAD STRIP  – pairs share an edge
    //   Vertex layout:  0-1
    //                   2-3
    //                   4-5 …
    //   Quad k uses: 2k, 2k+1, 2k+3, 2k+2
    // ---------------------------------------------------------------------------
    void tesselate_quad_strip(DrawScope& scope, const PathPoints& points)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 3 < n; i += 2) {
            uint32_t a = push_pt(scope, points, i + 0);
            uint32_t b = push_pt(scope, points, i + 1);
            uint32_t c = push_pt(scope, points, i + 3);
            uint32_t d = push_pt(scope, points, i + 2);
            draw_scope_push_triangle(scope, a, b, c);
            draw_scope_push_triangle(scope, a, c, d);
        }
    }

    // ---------------------------------------------------------------------------
    // TRIANGLES  – every 3 vertices → independent triangle
    // ---------------------------------------------------------------------------
    void tesselate_triangles(DrawScope& scope, const PathPoints& points)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 2 < n; i += 3) {
            uint32_t a = push_pt(scope, points, i + 0);
            uint32_t b = push_pt(scope, points, i + 1);
            uint32_t c = push_pt(scope, points, i + 2);
            draw_scope_push_triangle(scope, a, b, c);
        }
    }

    // ---------------------------------------------------------------------------
    // TRIANGLE STRIP  – sliding window of 3, alternate winding each step
    // ---------------------------------------------------------------------------
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

    // ---------------------------------------------------------------------------
    // TRIANGLE FAN  – vertex 0 is the hub
    // ---------------------------------------------------------------------------
    void tesselate_triangle_fan(DrawScope& scope, const PathPoints& points)
    {
        const size_t n = points.size;
        if (n < 3) return;

        uint32_t hub = push_pt(scope, points, 0);
        uint32_t prev = push_pt(scope, points, 1);

        for (size_t i = 2; i < n; ++i) {
            uint32_t cur = push_pt(scope, points, i);
            draw_scope_push_triangle(scope, hub, prev, cur);
            prev = cur;
        }
    }

    // ---------------------------------------------------------------------------
    // LINES  – fill each segment as a rectangle with caps
    //   Uses fill_segment which already handles Butt / Square / Round.
    // ---------------------------------------------------------------------------
    void tesselate_lines(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap cap)
    {
        const float halfW = strokeWeight * 0.5f;
        const size_t n = points.size;
        for (size_t i = 0; i + 1 < n; i += 2)
            fill_segment(scope, pt(points, i), pt(points, i + 1), uv(points, i), uv(points, i + 1), col(points, i), col(points, i + 1), halfW, cap);
    }

    // ---------------------------------------------------------------------------
    // LINE STRIP  – connected open polyline, each segment is a rectangle
    // ---------------------------------------------------------------------------
    void tesselate_line_strip(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap cap)
    {
        const float halfW = strokeWeight * 0.5f;
        const size_t n = points.size;
        for (size_t i = 0; i + 1 < n; ++i)
            fill_segment(scope, pt(points, i), pt(points, i + 1), uv(points, i), uv(points, i + 1), col(points, i), col(points, i + 1), halfW, cap);
    }

    // ---------------------------------------------------------------------------
    // LINE LOOP  – closed polyline, last segment connects back to first
    // ---------------------------------------------------------------------------
    void tesselate_line_loop(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap cap)
    {
        const float halfW = strokeWeight * 0.5f;
        const size_t n = points.size;
        for (size_t i = 0; i < n; ++i) {
            size_t j = (i + 1) % n;
            fill_segment(scope, pt(points, i), pt(points, j), uv(points, i), uv(points, j), col(points, i), col(points, j), halfW, cap);
        }
    }

    // ---------------------------------------------------------------------------
    // POLYGON  – arbitrary (possibly concave) filled polygon via libtess2
    // ---------------------------------------------------------------------------
    void tesselate_polygon(DrawScope& scope, const PathPoints& points)
    {
        tess_fill(scope, points);
    }

} // namespace p5

// ===========================================================================
// STROKE  (outline via generate_solid_stroke)
// ===========================================================================

namespace p5
{
    // Convenience: stroke a closed loop built from a fixed set of indices.
    template <size_t N>
    static void stroke_closed(DrawScope& scope, const PathPoints& src, const size_t (&idx)[N], float sw, StrokeCap cap, StrokeJoin join, float ml, float rjt)
    {
        SubPoints<N> sub;
        for (size_t i = 0; i < N; ++i) sub.set(i, src, idx[i]);
        auto p = sub.make(N);
        generate_solid_stroke(scope, p, sw, cap, join, ml, rjt, true);
    }

    // ---------------------------------------------------------------------------
    void stroke_quads(DrawScope& scope, const PathPoints& points, float sw, StrokeCap cap, StrokeJoin join, float ml, float rjt)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 3 < n; i += 4) {
            const size_t idx[4] = {i, i + 1, i + 2, i + 3};
            stroke_closed(scope, points, idx, sw, cap, join, ml, rjt);
        }
    }

    // ---------------------------------------------------------------------------
    void stroke_quad_strip(DrawScope& scope, const PathPoints& points, float sw, StrokeCap cap, StrokeJoin join, float ml, float rjt)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 3 < n; i += 2) {
            // Reorder to CCW quad: i, i+1, i+3, i+2
            const size_t idx[4] = {i, i + 1, i + 3, i + 2};
            stroke_closed(scope, points, idx, sw, cap, join, ml, rjt);
        }
    }

    // ---------------------------------------------------------------------------
    void stroke_triangles(DrawScope& scope, const PathPoints& points, float sw, StrokeCap cap, StrokeJoin join, float ml, float rjt)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 2 < n; i += 3) {
            const size_t idx[3] = {i, i + 1, i + 2};
            stroke_closed(scope, points, idx, sw, cap, join, ml, rjt);
        }
    }

    // ---------------------------------------------------------------------------
    void stroke_triangle_strip(DrawScope& scope, const PathPoints& points, float sw, StrokeCap cap, StrokeJoin join, float ml, float rjt)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 2 < n; ++i) {
            const size_t idx[3] = {i, i + 1, i + 2};
            stroke_closed(scope, points, idx, sw, cap, join, ml, rjt);
        }
    }

    // ---------------------------------------------------------------------------
    void stroke_triangle_fan(DrawScope& scope, const PathPoints& points, float sw, StrokeCap cap, StrokeJoin join, float ml, float rjt)
    {
        const size_t n = points.size;
        if (n < 3) return;
        for (size_t i = 2; i < n; ++i) {
            const size_t idx[3] = {0, i - 1, i};
            stroke_closed(scope, points, idx, sw, cap, join, ml, rjt);
        }
    }

    // ---------------------------------------------------------------------------
    // LINES  – independent segments, caps only (no joins between segments)
    // ---------------------------------------------------------------------------
    void stroke_lines(DrawScope& scope, const PathPoints& points, float sw, StrokeCap cap, float /*ml*/, float rjt)
    {
        const size_t n = points.size;
        for (size_t i = 0; i + 1 < n; i += 2) {
            SubPoints<2> sub;
            sub.set(0, points, i);
            sub.set(1, points, i + 1);
            auto p = sub.make(2);
            generate_solid_stroke(scope, p, sw, cap, StrokeJoin::Miter, 0.f, rjt,
                                  /*close=*/false);
        }
    }

    // ---------------------------------------------------------------------------
    void stroke_line_strip(DrawScope& scope, const PathPoints& points, float sw, StrokeCap cap, StrokeJoin join, float ml, float rjt)
    {
        if (points.size < 2) return;
        generate_solid_stroke(scope, points, sw, cap, join, ml, rjt,
                              /*close=*/false);
    }

    // ---------------------------------------------------------------------------
    // LINE LOOP – closed, no caps needed
    // ---------------------------------------------------------------------------
    void stroke_line_loop(DrawScope& scope, const PathPoints& points, float sw, StrokeJoin join, float ml, float rjt)
    {
        if (points.size < 2) return;
        generate_solid_stroke(scope, points, sw, StrokeCap::Butt, join, ml, rjt,
                              /*close=*/true);
    }

    // ---------------------------------------------------------------------------
    void stroke_polygon(DrawScope& scope, const PathPoints& points, float sw, StrokeCap cap, StrokeJoin join, float ml, float rjt)
    {
        if (points.size < 2) return;
        generate_solid_stroke(scope, points, sw, cap, join, ml, rjt,
                              /*close=*/true);
    }

} // namespace p5
