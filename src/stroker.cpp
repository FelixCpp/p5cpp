#include "stroker.hpp"
#include <limits>
#include <numbers>

inline static constexpr float PI = std::numbers::pi_v<float>;

namespace p5
{
    class DefaultStroker : public Stroker
    {
    public:
        void stroke(DrawScope& scope, const DrawPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, bool close) override
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

                const StrokeSegment segment = computeStrokeSegment(i, nextIndex, points, halfStrokeWeight);
                emitStrokeSegment(scope, segment, points);

                const bool needsJoin = !(isEnd) or close;
                if (needsJoin) {
                    const StrokeSegment previousSegment = computeStrokeSegment(prevIndex, i, points, halfStrokeWeight);
                    const StrokeSegment nextSegment = computeStrokeSegment(nextIndex, (nextIndex + 1) % n, points, halfStrokeWeight);
                    const StrokeCorner corner = computeStrokeCorner(previousSegment, segment, nextSegment, points, halfStrokeWeight, miterLimit);

                    emitStrokeJoin(scope, corner, points, halfStrokeWeight, strokeJoin);
                }

                // Insert stroke caps at the start and end of the stroke if needed
                if (not close) {
                    if (isStart) emitStrokeCap(scope, segment, points, strokeWeight, strokeCap, false);
                    if (isEnd) emitStrokeCap(scope, segment, points, strokeWeight, strokeCap, true);
                }
            }
        }

    private:
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

        StrokeCorner computeStrokeCorner(
            const StrokeSegment& previous,
            const StrokeSegment& current,
            const StrokeSegment& next,
            const DrawPoints& points,
            float halfStrokeWeight,
            float miterLimit
        )
        {
            const size_t cornerIndex = current.endIndex;

            // ---------------------------------------------------------------
            // 1. Drehrichtung an diesem Eckpunkt bestimmen
            //    cross > 0 → Linkskurve  (CCW turn) → outer liegt rechts
            //    cross < 0 → Rechtskurve (CW turn)  → outer liegt links
            // ---------------------------------------------------------------
            const float turn = cross(current.direction, next.direction);
            const bool leftTurn = (turn > 0.0f);

            // ---------------------------------------------------------------
            // 2. joinStart / joinEnd:
            //    Bei Linkskurve  ist outer = +normal-Seite
            //    Bei Rechtskurve ist outer = -normal-Seite
            //    → wir wählen einfach die korrekte Seite der Segmente
            // ---------------------------------------------------------------
            const float2 joinStart = !leftTurn ? current.innerEnd : current.outerEnd;
            const float2 joinEnd = !leftTurn ? next.innerStart : next.outerStart;

            // ---------------------------------------------------------------
            // 3. innerHit (M): Schnittpunkt der konkaven Seite
            // ---------------------------------------------------------------
            const float2 innerA = !leftTurn ? current.outerEnd : current.innerEnd;
            const float2 innerB = !leftTurn ? next.outerStart : next.innerStart;

            float2 innerHit;
            {
                const float2 AB = innerB - innerA;
                const float denom = cross(current.direction, next.direction);

                if ((std::abs(denom) < 1e-6f) or (turn < 0.1f))
                    innerHit = (innerA + innerB) * 0.5f;
                else
                    innerHit = innerA + current.direction * (cross(AB, next.direction) / denom);
            }

            // ---------------------------------------------------------------
            // 4. outerHit: Schnittpunkt der konvexen Seite (Miter-Spitze)
            // ---------------------------------------------------------------
            float2 outerHit;
            bool exceedsMiterLimit = false;
            {
                const float2 AB = joinEnd - joinStart;
                const float denom = cross(current.direction, next.direction); // selber denom wie oben

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

        StrokeSegment computeStrokeSegment(size_t startIndex, size_t endIndex, const DrawPoints& points, float halfStrokeWeight)
        {
            const float2& start = points.positions[startIndex];
            const float2& end = points.positions[endIndex];
            const float2 delta = end - start;
            const float2 direction = normalized(delta);
            // perp() dreht 90° — Konvention hier: normal zeigt nach "links" (inner/outside
            // hängt von Wicklungsrichtung ab; wir definieren:
            //   outer = start - normal * half  →  liegt auf der konvexen Seite
            //   inner = start + normal * half  →  liegt auf der konkaven Seite
            // Das war bereits korrekt, ABER: outer/inner sind relativ zur Umlaufrichtung,
            // nicht zur Geometrie. Die Implementierung bleibt gleich — der Aufrufer muss
            // auf konsistente Wicklungsrichtung (CCW/CW) achten.
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

        void emitStrokeSegment(DrawScope& scope, const StrokeSegment& segment, const DrawPoints& points)
        {
            const size_t vertexBase = scope.getVertexCount();
            const color_t startColor = points.colors[segment.startIndex];
            const color_t endColor = points.colors[segment.endIndex];

            push(scope, segment.innerStart, startColor);
            push(scope, segment.outerStart, startColor);
            push(scope, segment.outerEnd, endColor);
            push(scope, segment.innerEnd, endColor);

            // push(scope, segment.innerStart, color(255, 0, 0));
            // push(scope, segment.outerStart, color(0, 255, 0));
            // push(scope, segment.outerEnd, color(0, 255, 255));
            // push(scope, segment.innerEnd, color(255, 0, 255));

            push(scope, vertexBase + 0);
            push(scope, vertexBase + 1);
            push(scope, vertexBase + 2);
            push(scope, vertexBase + 2);
            push(scope, vertexBase + 3);
            push(scope, vertexBase + 0);
        }

        void emitStrokeCap(DrawScope& scope, const StrokeSegment& segment, const DrawPoints& points, float strokeWeight, StrokeCap strokeCap, bool isEnd)
        {
            switch (strokeCap) {
                case StrokeCap::square: emitSquareCap(scope, segment, points, strokeWeight, isEnd); break;
                case StrokeCap::round: emitRoundCap(scope, segment, points, strokeWeight, isEnd); break;
                case StrokeCap::butt:
                    // No additional geometry needed for butt cap
                    break;
            }
        }

        void emitSquareCap(DrawScope& scope, const StrokeSegment& segment, const DrawPoints& points, float strokeWeight, bool isEnd)
        {
            const size_t vertexBase = scope.getVertexCount();
            const color_t color = points.colors[isEnd ? segment.endIndex : segment.startIndex];

            const float2 offset = segment.direction * strokeWeight;
            const float2 capStart = isEnd ? segment.outerEnd : segment.outerStart;
            const float2 capEnd = isEnd ? segment.outerEnd + offset : segment.outerStart - offset;
            const float2 innerCapStart = isEnd ? segment.innerEnd : segment.innerStart;
            const float2 innerCapEnd = isEnd ? segment.innerEnd + offset : segment.innerStart - offset;

            push(scope, innerCapStart, color);
            push(scope, capStart, color);
            push(scope, capEnd, color);
            push(scope, innerCapEnd, color);

            push(scope, vertexBase + 0);
            push(scope, vertexBase + 1);
            push(scope, vertexBase + 2);
            push(scope, vertexBase + 2);
            push(scope, vertexBase + 3);
            push(scope, vertexBase + 0);
        }

        void emitRoundCap(DrawScope& scope, const StrokeSegment& segment, const DrawPoints& points, float strokeWeight, bool isEnd)
        {
            const size_t vertexBase = scope.getVertexCount();
            const color_t color = points.colors[isEnd ? segment.endIndex : segment.startIndex];
            const float2 center = points.positions[isEnd ? segment.endIndex : segment.startIndex];
            const float2 direction = isEnd ? segment.direction : -segment.direction;
            const float radius = strokeWeight * 0.5f;
            const size_t segmentCount = computeCircleSegmentCount(PI, radius);
            const float startAngle = std::atan2(direction.y, direction.x) - PI / 2.0f;

            // Insert center vertex for the round cap
            push(scope, center, color);

            for (size_t i = 0; i <= segmentCount; ++i) {
                float angle = startAngle + static_cast<float>(i) / static_cast<float>(segmentCount) * PI;
                float2 offset = float2 {std::cos(angle), std::sin(angle)} * radius;
                push(scope, center + offset, color);
            }

            for (size_t i = 0; i < segmentCount; ++i) {
                push(scope, vertexBase + 0);
                push(scope, vertexBase + i + 1);
                push(scope, vertexBase + i + 2);
            }
        }

        void emitStrokeJoin(DrawScope& scope, const StrokeCorner& corner, const DrawPoints& points, float halfStrokeWeight, StrokeJoin strokeJoin)
        {
            if ((strokeJoin == StrokeJoin::bevel) or (strokeJoin == StrokeJoin::miter and corner.exceedsMiterLimit)) {
                // Fallback to bevel join if miter limit is exceeded
                return emitBevelJoin(scope, corner, points);
            }

            if (strokeJoin == StrokeJoin::miter) {
                return emitMiterJoin(scope, corner, points);
            }

            if (strokeJoin == StrokeJoin::round) {
                return emitRoundJoin(scope, corner, points, halfStrokeWeight);
            }
        }

        void emitBevelJoin(DrawScope& scope, const StrokeCorner& corner, const DrawPoints& points)
        {
            const size_t vertexBase = scope.getVertexCount();
            const color_t color = points.colors[corner.index];

            push(scope, corner.innerHit, color);
            push(scope, corner.joinStart, color);
            push(scope, corner.joinEnd, color);

            push(scope, vertexBase + 0);
            push(scope, vertexBase + 1);
            push(scope, vertexBase + 2);
        }

        void emitMiterJoin(DrawScope& scope, const StrokeCorner& corner, const DrawPoints& points)
        {
            const size_t vertexBase = scope.getVertexCount();
            const color_t color = points.colors[corner.index];

            push(scope, corner.innerHit, color);
            push(scope, corner.joinStart, color);
            push(scope, corner.outerHit, color);
            push(scope, corner.joinEnd, color);

            push(scope, vertexBase + 0);
            push(scope, vertexBase + 1);
            push(scope, vertexBase + 2);
            push(scope, vertexBase + 2);
            push(scope, vertexBase + 3);
            push(scope, vertexBase + 0);
        }

        void emitRoundJoin(DrawScope& scope, const StrokeCorner& corner, const DrawPoints& points, float halfStrokeWeight)
        {
            const color_t color = points.colors[corner.index];
            const float2 cornerPos = points.positions[corner.index];

            // Winkel von joinStart und joinEnd relativ zum Eckpunkt bestimmen
            const float2 startDir = normalized(corner.joinStart - cornerPos);
            const float2 endDir = normalized(corner.joinEnd - cornerPos);

            float angleStart = std::atan2(startDir.y, startDir.x);
            float angleEnd = std::atan2(endDir.y, endDir.x);

            // Sicherstellen dass wir den kurzen Bogen nehmen (< 180°)
            float angleDiff = angleEnd - angleStart;
            if (angleDiff > M_PI) angleDiff -= 2.0f * M_PI;
            else if (angleDiff < -M_PI) angleDiff += 2.0f * M_PI;

            // Anzahl Segmente je nach Bogengröße und Radius
            // ~1 Segment pro 5° ist ein guter Kompromiss zwischen Qualität und Performance
            // const int steps = std::max(1, static_cast<int>(std::abs(angleDiff) / (5.0f * M_PI / 180.0f)));
            const size_t steps = computeCircleSegmentCount(std::abs(angleDiff), halfStrokeWeight);

            const size_t vertexBase = scope.getVertexCount();

            // Zentrum (innerHit) einmal pushen
            push(scope, corner.innerHit, color); // Index: vertexBase + 0

            // Fächer-Vertices entlang des Bogens
            for (int i = 0; i <= steps; i++) {
                const float t = static_cast<float>(i) / static_cast<float>(steps);
                const float angle = angleStart + angleDiff * t;
                const float2 pos = cornerPos + float2 {std::cos(angle), std::sin(angle)} * halfStrokeWeight;
                push(scope, pos, color); // Index: vertexBase + 1 + i
            }

            // Dreiecke als Fächer um innerHit
            for (int i = 0; i < steps; i++) {
                push(scope, vertexBase + 0);     // innerHit
                push(scope, vertexBase + 1 + i); // aktueller Bogenpunkt
                push(scope, vertexBase + 2 + i); // nächster Bogenpunkt
            }
        }

        void push(DrawScope& scope, const float2& position, color_t color)
        {
            const float4 col = float4 {
                .x = static_cast<float>(red(color)) / 255.0f,
                .y = static_cast<float>(green(color)) / 255.0f,
                .z = static_cast<float>(blue(color)) / 255.0f,
                .w = static_cast<float>(alpha(color)) / 255.0f,
            };

            scope.push(Vertex {.position = position, .texcoord = float2 {}, .color = col});
        }

        void push(DrawScope& scope, uint32_t index)
        {
            scope.push(index);
        }

        std::optional<float2> lineIntersection(const float2& p1, const float2& p2, const float2& p3, const float2& p4)
        {
            const float2 d1 = p2 - p1;
            const float2 d2 = p4 - p3;

            const float cross = d1.x * d2.y - d1.y * d2.x;
            if (std::abs(cross) < std::numeric_limits<float>::epsilon()) {
                return std::nullopt; // Lines are parallel
            }

            const float t = ((p3.x - p1.x) * d2.y - (p3.y - p1.y) * d2.x) / cross;
            return p1 + d1 * t;
        }
    };
} // namespace p5

namespace p5
{
    std::unique_ptr<Stroker> createStroker()
    {
        return std::make_unique<DefaultStroker>();
    }
} // namespace p5
