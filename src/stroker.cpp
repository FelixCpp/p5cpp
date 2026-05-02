#include "stroker.hpp"

#include <algorithm>
#include <utility>

namespace p5
{
    class DefaultStroker : public Stroker
    {
    public:
        void stroke(DrawScope& scope, const DrawPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, StrokeAlign strokeAlign, float miterLimit, bool close) override
        {
            const size_t n = points.size;
            if (n < 2) return;

            const size_t segmentCount = close ? n : (n - 1);
            const float half = strokeWeight * 0.5f;

            StrokeCorner prevCorner = stroke_corner_create(points.positions[(n - 2) % n], points.positions[points.size - 1], points.positions[0 % n], strokeWeight, miterLimit);

            for (size_t i = 0; i < segmentCount; ++i) {
                const size_t prevIndex = (i + n - 1) % n;
                const size_t currIndex = i;
                const size_t nextIndex = (i + 1) % n;

                const float2& prevPos = points.positions[prevIndex];
                const float2& currPos = points.positions[currIndex];
                const float2& nextPos = points.positions[nextIndex];

                StrokeCorner currCorner = stroke_corner_create(prevPos, currPos, nextPos, strokeWeight, miterLimit);

                const color_t prevColor = points.colors[prevIndex];
                const color_t currColor = points.colors[currIndex];

                { // Insert segment
                    // const uint32_t baseVertexIndex = scope.getVertexCount();
                    //
                    // push(scope, cornerOuter(prevCorner, true), prevColor);
                    // push(scope, cornerInner(prevCorner, true), prevColor);
                    // push(scope, cornerInner(currCorner, false), currColor);
                    // push(scope, cornerOuter(currCorner, false), currColor);
                    //
                    // push(scope, baseVertexIndex + 0);
                    // push(scope, baseVertexIndex + 1);
                    // push(scope, baseVertexIndex + 2);
                    // push(scope, baseVertexIndex + 2);
                    // push(scope, baseVertexIndex + 3);
                    // push(scope, baseVertexIndex + 0);

                    // Build a segment
                    float2 dir = normalized(nextPos - currPos);
                    float2 normal = perp(dir);
                    float2 segStartOuter = currPos + normal * half;
                    float2 segStartInner = currPos - normal * half;
                    float2 segEndOuter = nextPos + normal * half;
                    float2 segEndInner = nextPos - normal * half;

                    const uint32_t baseVertexIndex = scope.getVertexCount();
                    push(scope, segStartOuter, prevColor);
                    push(scope, segStartInner, prevColor);
                    push(scope, segEndInner, currColor);
                    push(scope, segEndOuter, currColor);

                    push(scope, baseVertexIndex + 0);
                    push(scope, baseVertexIndex + 1);
                    push(scope, baseVertexIndex + 2);
                    push(scope, baseVertexIndex + 2);
                    push(scope, baseVertexIndex + 3);
                    push(scope, baseVertexIndex + 0);
                }

                if (true) { // Join
                    if ((strokeJoin == StrokeJoin::bevel) or (strokeJoin == StrokeJoin::miter and currCorner.miterLimitExceeded)) {
                        const uint32_t baseVertexIndex = scope.getVertexCount();
                        push(scope, currCorner.innerIntersection, currColor);
                        push(scope, currCorner.joinStart, currColor);
                        push(scope, currCorner.joinEnd, currColor);

                        push(scope, baseVertexIndex + 0);
                        push(scope, baseVertexIndex + 1);
                        push(scope, baseVertexIndex + 2);
                    } else if (strokeJoin == StrokeJoin::miter) {
                        const uint32_t baseVertexIndex = scope.getVertexCount();
                        push(scope, currCorner.innerIntersection, currColor);
                        push(scope, currCorner.joinStart, currColor);
                        push(scope, currCorner.outerIntersection, currColor);
                        push(scope, currCorner.joinEnd, currColor);

                        push(scope, baseVertexIndex + 0);
                        push(scope, baseVertexIndex + 1);
                        push(scope, baseVertexIndex + 2);
                        push(scope, baseVertexIndex + 2);
                        push(scope, baseVertexIndex + 3);
                        push(scope, baseVertexIndex + 0);
                    } else if (strokeJoin == StrokeJoin::round) {
                        const uint32_t baseVertexIndex = scope.getVertexCount();
                        constexpr size_t segmentCount = 16; // TODO: adapt based on angle

                        float angleStart = std::atan2(currCorner.joinStart.y - currCorner.origin.y, currCorner.joinStart.x - currCorner.origin.x);
                        float angleEnd = std::atan2(currCorner.joinEnd.y - currCorner.origin.y, currCorner.joinEnd.x - currCorner.origin.x);

                        // Differenz auf (-π, π] normalisieren → immer der kurze Bogen
                        float delta = angleEnd - angleStart;
                        if (delta > M_PI) delta -= 2.0f * M_PI;
                        if (delta < -M_PI) delta += 2.0f * M_PI;

                        // if (delta == 0.0f) {
                        std::fprintf(stdout, "Hello %.2f\n", delta);
                        std::fflush(stdout);
                        // }

                        // Segmentanzahl dynamisch nach Winkelgröße (optional aber sinnvoll)
                        // const size_t segmentCount = std::max(1, (int)std::ceil(std::abs(delta) / (10.0f * M_PI / 180.0f)));

                        // const uint32_t baseVertexIndex = scope.getVertexCount();
                        push(scope, currCorner.innerIntersection, currColor);
                        for (size_t j = 0; j <= segmentCount; ++j) {
                            float t = static_cast<float>(j) / static_cast<float>(segmentCount);
                            float angle = angleStart + t * delta; // funktioniert für CW und CCW
                            push(scope, float2 {currCorner.origin.x + std::cos(angle) * half, currCorner.origin.y + std::sin(angle) * half}, currColor);
                        }
                        for (size_t j = 0; j < segmentCount; ++j) {
                            push(scope, baseVertexIndex + 0);
                            push(scope, baseVertexIndex + j + 1);
                            push(scope, baseVertexIndex + j + 2);
                        }
                    }
                }

                prevCorner = currCorner;
            }
        }

    private:
        inline static void push(DrawScope& scope, const float2& position, const color_t& color)
        {
            float4 colorVec = float4 {
                .x = static_cast<float>(red(color)) / 255.0f,
                .y = static_cast<float>(green(color)) / 255.0f,
                .z = static_cast<float>(blue(color)) / 255.0f,
                .w = static_cast<float>(alpha(color)) / 255.0f,
            };

            scope.push(Vertex {.position = position, .texcoord = {0.0f, 0.0f}, .color = colorVec});
        }

        inline static void push(DrawScope& scope, uint32_t index)
        {
            scope.push(index);
        }

        struct StrokeCorner
        {
            float2 origin;            // Origin point where two segments meet
            float2 innerIntersection; // Intersection point of the inner edges of the stroke.
            float2 outerIntersection; // Intersection point of the outer edges of the stroke, calculated using the incoming segment.
            float2 joinStart;         // The point on the outer edge of the incoming segment where the join starts.
            float2 joinEnd;           // Same as joinStart, but from the perspective of the outgoing segment.
            bool miterLimitExceeded;  // Indicates whether the miter is valid (i.e., within the miter limit)
            bool isConcave;           // Indicates whether the corner is a concave turn (used for bevel joins)
        };

        inline static const float2& cornerOuter(const StrokeCorner& corner, bool isStart)
        {
            if (isStart) return corner.isConcave ? corner.innerIntersection : corner.joinEnd;
            else return corner.isConcave ? corner.innerIntersection : corner.joinStart;
        }

        inline static const float2& cornerInner(const StrokeCorner& corner, bool isStart)
        {
            if (isStart) return corner.isConcave ? corner.joinEnd : corner.innerIntersection;
            else return corner.isConcave ? corner.joinStart : corner.innerIntersection;
        }

        static StrokeCorner stroke_corner_create(const float2& prev, const float2& curr, const float2& next, float strokeWeight, float miterLimit)
        {
            const float half = strokeWeight * 0.5f;

            const float2 dIn = normalized(curr - prev);
            const float2 dOut = normalized(next - curr);
            const float2 nIn = perp(dIn);
            const float2 nOut = perp(dOut);

            const float crossVal = p5::cross(dIn, dOut);
            const bool leftTurn = (crossVal >= 0.0f);
            const float outerSign = leftTurn ? -1.0f : 1.0f;
            const float innerSign = -outerSign;

            float2 joinStart = curr + nIn * (half * outerSign);
            float2 joinEnd = curr + nOut * (half * outerSign);
            const float2 innerBase = curr + nIn * (half * innerSign);

            const float2 innerIntersection = line_intersect(innerBase, dIn, curr + nOut * (half * innerSign), dOut)
                                                 .transform([&curr, miterLimit, half](const float2& intersection) {
                                                     float2 toInner = intersection - curr;
                                                     float dist = length(toInner);
                                                     if (dist > miterLimit * half)
                                                         return curr + (toInner / dist) * (miterLimit * half);
                                                     return intersection;
                                                 })
                                                 .value_or(innerBase);

            //                                    if (std::optional unlimitedInnerIntersection = line_intersect(
            //                                            innerBase,
            //                                            dIn,
            //                                            curr + nOut * (half * innerSign),
            //                                            dOut
            //                                        ))
            // {
            //     float2 toInner = unlimitedInnerIntersection.value() - curr;
            //     float dist = length(toInner);
            //     if (dist > miterLimit * half)
            //         innerIntersection = curr + (toInner / dist) * (miterLimit * half);
            //     else
            //         innerIntersection = unlimitedInnerIntersection.value(); // <-- fehlte
            // }
            // else
            // {
            //     innerIntersection = innerBase;
            // }

            float2 outerIntersection = line_intersect(joinStart, dIn, joinEnd, dOut)
                                           .value_or(joinStart);

            const bool miterLimitExceeded = lengthSquared(outerIntersection - curr) > (miterLimit * half) * (miterLimit * half);
            if (miterLimitExceeded) outerIntersection = curr;

            // if (!leftTurn) // konkav
            // {
            //     // Rollen tauschen damit das Quad immer gleich aufgebaut werden kann
            //     std::swap(innerIntersection, outerIntersection);
            //     std::swap(joinStart, joinEnd);
            // }

            return StrokeCorner {
                .origin = curr,
                .innerIntersection = innerIntersection,
                .outerIntersection = outerIntersection,
                .joinStart = joinStart,
                .joinEnd = joinEnd,
                .miterLimitExceeded = miterLimitExceeded,
                .isConcave = (crossVal < 0.0f)
            };
        }

        inline static std::optional<float2> line_intersect(float2 p, float2 Da, float2 Q, float2 Db)
        {
            const float denom = cross(Da, Db);
            if (std::abs(denom) < 1e-6f) return std::nullopt; // parallel
            const float t = cross(Q - p, Db) / denom;
            return p + Da * t;
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
