#include "stroker.hpp"
#include <optional>

namespace p5cpp
{
    struct StrokeCorner
    {
        size_t index;
        float2 joinStart;
        float2 joinEnd;
        float2 innerHit;
        float2 outerHit;
        bool exceedsMiterLimit;
    };

    struct StrokeSegment
    {
        size_t startIndex;
        size_t endIndex;
        float2 outerStart;
        float2 innerStart;
        float2 outerEnd;
        float2 innerEnd;
        float2 direction;
        float2 normal;
    };

    namespace caps
    {
        static void emit_stroke_cap(DrawScope& scope, const StrokeSegment& segment, const PathPoints& points, float strokeWeight, StrokeCapStyle strokeCap, bool isEnd);
        static void emit_square_cap(DrawScope& scope, const StrokeSegment& segment, const PathPoints& points, float strokeWeight, bool isEnd);
        static void emit_round_cap(DrawScope& scope, const StrokeSegment& segment, const PathPoints& points, float strokeWeight, bool isEnd);
    } // namespace caps

    namespace joins
    {
        static void emit_stroke_join(DrawScope& scope, const StrokeCorner& corner, const PathPoints& points, float halfStrokeWeight, float roundJoinAngleThreshold, StrokeJoin strokeJoin);
        static void emit_bevel_join(DrawScope& scope, const StrokeCorner& corner, const PathPoints& points);
        static void emit_miter_join(DrawScope& scope, const StrokeCorner& corner, const PathPoints& points);
        static void emit_round_join(DrawScope& scope, const StrokeCorner& corner, const PathPoints& points, float halfStrokeWeight, float roundJoinAngleThreshold);
    } // namespace joins

    static StrokeCorner compute_stroke_corner(const StrokeSegment& previous, const StrokeSegment& current, const StrokeSegment& next, const PathPoints& points, float halfStrokeWeight, float miterLimit);
    static StrokeSegment compute_stroke_segment(size_t startIndex, size_t endIndex, const PathPoints& points, float halfStrokeWeight);
    static void emit_stroke_segment(DrawScope& scope, const StrokeSegment& segment, const PathPoints& points);

    static void push_vertex(DrawScope& scope, const float2& position, color_t color);
    static void push_triangle(DrawScope& scope, uint32_t a, uint32_t b, uint32_t c);
    static std::optional<float2> line_intersection(const float2& p1, const float2& d1, const float2& p2, const float2& d2);
} // namespace p5cpp

namespace p5cpp::caps
{
    static void emit_stroke_cap(DrawScope& scope, const StrokeSegment& segment, const PathPoints& points, float strokeWeight, StrokeCapStyle strokeCap, bool isEnd)
    {
        switch (strokeCap) {
            case StrokeCapStyle::square: emit_square_cap(scope, segment, points, strokeWeight, isEnd); break;
            case StrokeCapStyle::round: emit_round_cap(scope, segment, points, strokeWeight, isEnd); break;
            case StrokeCapStyle::butt:
                // No additional geometry needed for butt cap
                break;
        }
    }

    void emit_square_cap(DrawScope& scope, const StrokeSegment& segment, const PathPoints& points, float strokeWeight, bool isEnd)
    {
        const color_t color = points.colors[isEnd ? segment.endIndex : segment.startIndex];

        const float2 offset = segment.direction * strokeWeight;
        const float2 capStart = isEnd ? segment.outerEnd : segment.outerStart;
        const float2 capEnd = isEnd ? segment.outerEnd + offset : segment.outerStart - offset;
        const float2 innerCapStart = isEnd ? segment.innerEnd : segment.innerStart;
        const float2 innerCapEnd = isEnd ? segment.innerEnd + offset : segment.innerStart - offset;

        const size_t baseVertex = scope.vertexCursor - scope.baseVertex;

        push_vertex(scope, innerCapStart, color);
        push_vertex(scope, capStart, color);
        push_vertex(scope, capEnd, color);
        push_vertex(scope, innerCapEnd, color);

        push_triangle(scope, baseVertex + 0, baseVertex + 1, baseVertex + 2);
        push_triangle(scope, baseVertex + 2, baseVertex + 3, baseVertex + 0);
    }

    void emit_round_cap(DrawScope& scope, const StrokeSegment& segment, const PathPoints& points, float strokeWeight, bool isEnd)
    {
        const color_t color = points.colors[isEnd ? segment.endIndex : segment.startIndex];
        const float2 center = points.positions[isEnd ? segment.endIndex : segment.startIndex];
        const float2 direction = isEnd ? segment.direction : -segment.direction;
        const float radius = strokeWeight * 0.5f;
        const size_t segmentCount = computeCircleSegmentCount(PI, radius);
        const float startAngle = std::atan2(direction.y, direction.x) - PI / 2.0f;

        // Insert center vertex for the round cap
        const size_t centerIndex = scope.vertexCursor - scope.baseVertex;
        push_vertex(scope, center, color);

        for (size_t i = 0; i <= segmentCount; ++i) {
            float angle = startAngle + static_cast<float>(i) / static_cast<float>(segmentCount) * PI;
            float2 offset = float2 {std::cos(angle), std::sin(angle)} * radius;
            push_vertex(scope, center + offset, color);
        }

        for (size_t i = 0; i < segmentCount; ++i) {
            push_triangle(scope, centerIndex + 0, centerIndex + i + 1, centerIndex + i + 2);
        }
    }
} // namespace p5cpp::caps

namespace p5cpp::joins
{
    static void emit_stroke_join(DrawScope& scope, const StrokeCorner& corner, const PathPoints& points, float halfStrokeWeight, float roundJoinAngleThreshold, StrokeJoin strokeJoin)
    {
        switch (strokeJoin) {
            case StrokeJoin::bevel: emit_bevel_join(scope, corner, points); break;
            case StrokeJoin::miter: emit_miter_join(scope, corner, points); break;
            case StrokeJoin::round: emit_round_join(scope, corner, points, halfStrokeWeight, roundJoinAngleThreshold); break;
        }
    }

    static void emit_bevel_join(DrawScope& scope, const StrokeCorner& corner, const PathPoints& points)
    {
        const color_t color = points.colors[corner.index];

        const size_t baseVertex = scope.vertexCursor - scope.baseVertex;
        push_vertex(scope, corner.innerHit, color);
        push_vertex(scope, corner.joinStart, color);
        push_vertex(scope, corner.joinEnd, color);

        push_triangle(scope, baseVertex + 0, baseVertex + 1, baseVertex + 2);
    }

    static void emit_miter_join(DrawScope& scope, const StrokeCorner& corner, const PathPoints& points)
    {
        if (corner.exceedsMiterLimit) {
            // Fallback to bevel join if miter limit is exceeded
            return emit_bevel_join(scope, corner, points);
        }

        const size_t baseVertex = scope.vertexCursor - scope.baseVertex;
        const color_t color = points.colors[corner.index];

        push_vertex(scope, corner.innerHit, color);
        push_vertex(scope, corner.joinStart, color);
        push_vertex(scope, corner.outerHit, color);
        push_vertex(scope, corner.joinEnd, color);

        push_triangle(scope, baseVertex + 0, baseVertex + 1, baseVertex + 2);
        push_triangle(scope, baseVertex + 2, baseVertex + 3, baseVertex + 0);
    }

    static void emit_round_join(DrawScope& scope, const StrokeCorner& corner, const PathPoints& points, float halfStrokeWeight, float roundJoinAngleThreshold)
    {
        const color_t color = points.colors[corner.index];
        const float2 cornerPos = points.positions[corner.index];

        const float2 startDir = normalized(corner.joinStart - cornerPos);
        const float2 endDir = normalized(corner.joinEnd - cornerPos);

        float angleStart = std::atan2(startDir.y, startDir.x);
        float angleEnd = std::atan2(endDir.y, endDir.x);

        float angleDiff = angleEnd - angleStart;
        if (angleDiff > PI) angleDiff -= 2.0f * PI;
        else if (angleDiff < -PI) angleDiff += 2.0f * PI;

        if (std::abs(angleDiff) < roundJoinAngleThreshold) {
            return emit_bevel_join(scope, corner, points);
        }

        const size_t steps = computeCircleSegmentCount(std::abs(angleDiff), halfStrokeWeight);

        const size_t baseVertex = scope.vertexCursor - scope.baseVertex;
        push_vertex(scope, cornerPos, color); // Index: vertexBase + 0

        for (int i = 0; i <= steps; i++) {
            const float t = static_cast<float>(i) / static_cast<float>(steps);
            const float angle = angleStart + angleDiff * t;
            const float2 pos = cornerPos + float2 {std::cos(angle), std::sin(angle)} * halfStrokeWeight;
            push_vertex(scope, pos, color); // Index: vertexBase + 1 + i
        }

        // Dreiecke als Fächer um innerHit
        for (int i = 0; i < steps; i++) {
            push_triangle(
                scope,
                baseVertex + 0,     // innerHit
                baseVertex + 1 + i, // aktueller Bogenpunkt
                baseVertex + 2 + i  // nächster Bogenpunkt
            );
        }
    }
} // namespace p5cpp::joins

namespace p5cpp
{
    StrokeCorner compute_stroke_corner(
        const StrokeSegment& previous,
        const StrokeSegment& current,
        const StrokeSegment& next,
        const PathPoints& points,
        float halfStrokeWeight,
        float miterLimit
    )
    {
        const size_t cornerIndex = current.endIndex;

        const float turn = cross(current.direction, next.direction);
        const bool leftTurn = (turn > 0.0f);

        const float2 joinStart = !leftTurn ? current.innerEnd : current.outerEnd;
        const float2 joinEnd = !leftTurn ? next.innerStart : next.outerStart;

        // innerHit is the base vertex of the join triangle on the concave side.
        // Always use the corner position: the concave-side region is already covered
        // by the two adjacent segment quads, so only the convex-side gap needs filling.
        const float2 innerHit = points.positions[cornerIndex];

        float2 outerHit;
        bool exceedsMiterLimit = false;
        {
            const float2 AB = joinEnd - joinStart;
            const float denom = cross(current.direction, next.direction);

            if (std::abs(denom) < 1e-6f) {
                outerHit = (joinStart + joinEnd) * 0.5f;
                exceedsMiterLimit = true;
            } else {
                outerHit = joinStart + current.direction * (cross(AB, next.direction) / denom);

                const float miterLength = length(outerHit - innerHit);
                exceedsMiterLimit = (miterLength / halfStrokeWeight) > miterLimit;
            }
        }

        return StrokeCorner {
            .index = cornerIndex,
            .joinStart = joinStart,
            .joinEnd = joinEnd,
            .innerHit = innerHit,
            .outerHit = outerHit,
            .exceedsMiterLimit = exceedsMiterLimit,
        };
    }

    static StrokeSegment compute_stroke_segment(size_t startIndex, size_t endIndex, const PathPoints& points, float halfStrokeWeight)
    {
        const float2& start = points.positions[startIndex];
        const float2& end = points.positions[endIndex];
        const float2 delta = end - start;
        const float2 direction = normalized(delta);
        const float2 normal = perp(direction);
        return StrokeSegment {
            .startIndex = startIndex,
            .endIndex = endIndex,
            .outerStart = start - normal * halfStrokeWeight,
            .innerStart = start + normal * halfStrokeWeight,
            .outerEnd = end - normal * halfStrokeWeight,
            .innerEnd = end + normal * halfStrokeWeight,
            .direction = direction,
            .normal = normal,
        };
    }

    static void emit_stroke_segment(DrawScope& scope, const StrokeSegment& segment, const PathPoints& points)
    {
        const color_t startColor = points.colors[segment.startIndex];
        const color_t endColor = points.colors[segment.endIndex];

        const size_t baseVertex = scope.vertexCursor - scope.baseVertex;
        push_vertex(scope, segment.innerStart, startColor);
        push_vertex(scope, segment.outerStart, startColor);
        push_vertex(scope, segment.outerEnd, endColor);
        push_vertex(scope, segment.innerEnd, endColor);

        push_triangle(scope, baseVertex + 0, baseVertex + 1, baseVertex + 2);
        push_triangle(scope, baseVertex + 2, baseVertex + 3, baseVertex + 0);
    }
} // namespace p5cpp

namespace p5cpp
{
    static void push_vertex(DrawScope& scope, const float2& position, color_t color)
    {
        const float4 col = float4 {
            .x = static_cast<float>(red(color)) / 255.0f,
            .y = static_cast<float>(green(color)) / 255.0f,
            .z = static_cast<float>(blue(color)) / 255.0f,
            .w = static_cast<float>(alpha(color)) / 255.0f,
        };

        draw_scope_push_vertex(scope, position, float2 {}, col);
    }

    static void push_triangle(DrawScope& scope, uint32_t a, uint32_t b, uint32_t c)
    {
        draw_scope_push_triangle(scope, a, b, c);
    }

    static std::optional<float2> line_intersection(const float2& p1, const float2& p2, const float2& p3, const float2& p4)
    {
        const float2 d1 = p2 - p1;
        const float2 d2 = p4 - p3;

        const float cross = d1.x * d2.y - d1.y * d2.x;
        if (std::abs(cross) < std::numeric_limits<float>::epsilon()) {
            return std::nullopt;
        }

        const float t = ((p3.x - p1.x) * d2.y - (p3.y - p1.y) * d2.x) / cross;
        return p1 + d1 * t;
    }
} // namespace p5cpp

namespace p5cpp
{
    void generate_solid_stroke(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinAngleThreshold, bool close)
    {
        const size_t n = points.size;
        if (n < 2) return; // NOTE: There's nothing to stroke if there are less than 2 points

        const float halfStrokeWeight = strokeWeight * 0.5f;

        const size_t segmentCount = close ? n : n - 1;

        for (size_t i = 0; i < segmentCount; ++i) {
            const size_t prevIndex = (i + n - 1) % n; // NOTE: Works due to integral promotion rules
            const size_t nextIndex = (i + 1) % n;

            const float2& prev = points.positions[prevIndex];
            const float2& current = points.positions[i];
            const float2& next = points.positions[nextIndex];

            const bool isStart = (i == 0);
            const bool isEnd = (i == segmentCount - 1);

            const StrokeSegment segment = compute_stroke_segment(i, nextIndex, points, halfStrokeWeight);
            emit_stroke_segment(scope, segment, points);

            const bool needsJoin = !(isEnd) or close;
            if (needsJoin) {
                const StrokeSegment previousSegment = compute_stroke_segment(prevIndex, i, points, halfStrokeWeight);
                const StrokeSegment nextSegment = compute_stroke_segment(nextIndex, (nextIndex + 1) % n, points, halfStrokeWeight);
                const StrokeCorner corner = compute_stroke_corner(previousSegment, segment, nextSegment, points, halfStrokeWeight, miterLimit);

                joins::emit_stroke_join(scope, corner, points, halfStrokeWeight, roundJoinAngleThreshold, strokeJoin);
            }

            // Insert stroke caps at the start and end of the stroke if needed
            if (not close) {
                if (isStart) caps::emit_stroke_cap(scope, segment, points, strokeWeight, strokeCap.start, false);
                if (isEnd) caps::emit_stroke_cap(scope, segment, points, strokeWeight, strokeCap.end, true);
            }
        }
    }
} // namespace p5cpp
