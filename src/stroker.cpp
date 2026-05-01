#include "stroker.hpp"

namespace p5
{
    class DefaultStroker : public Stroker
    {
    public:
        void stroke(DrawScope& scope, const DrawPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, StrokeAlign strokeAlign, float miterLimit, bool close) override
        {
            const size_t n = points.size;

            // Early out if there are too few points to form a stroke.
            if (n < 2) return;

            // Determine the number of segments to process. If the path is closed, we need to process all n segments (including the one that connects the last point back to the first).
            // If it's open, we only process n-1 segments.
            const size_t segmentCount = close ? n : n - 1;

            for (size_t i = 0; i < segmentCount; ++i) {
                const float2& prev = points.positions[(i + n - 1) % n]; // Previous point (wrap around for closed paths)
                const float2& curr = points.positions[i];
                const float2& next = points.positions[(i + 1) % n]; // Wrap around for closed paths

                {
                    const float2 dir = normalized(next - curr);
                    const float2 normal = perp(dir);
                    const float2 halfStroke = normal * (strokeWeight / 2.0f);

                    const uint32_t vertexBase = scope.getVertexCount();
                    scope.push(Vertex {.position = curr + halfStroke, .texcoord = {}, .color = {1.0f, 1.0f, 1.0f, 1.0f}});
                    scope.push(Vertex {.position = curr - halfStroke, .texcoord = {}, .color = {1.0f, 1.0f, 1.0f, 1.0f}});
                    scope.push(Vertex {.position = next - halfStroke, .texcoord = {}, .color = {1.0f, 1.0f, 1.0f, 1.0f}});
                    scope.push(Vertex {.position = next + halfStroke, .texcoord = {}, .color = {1.0f, 1.0f, 1.0f, 1.0f}});
                    scope.push(vertexBase + 0);
                    scope.push(vertexBase + 1);
                    scope.push(vertexBase + 2);
                    scope.push(vertexBase + 0);
                    scope.push(vertexBase + 2);
                    scope.push(vertexBase + 3);
                }

                {
                    StrokeCorner corner = stroke_corner_create(prev, curr, next, strokeWeight, miterLimit);
                    const uint32_t vertexBase = scope.getVertexCount();
                    scope.push(Vertex {.position = corner.innerHit, .texcoord = {}, .color = {1.0f, 1.0f, 1.0f, 1.0f}});
                    scope.push(Vertex {.position = corner.outerHitIn, .texcoord = {}, .color = {1.0f, 1.0f, 1.0f, 1.0f}});
                    scope.push(Vertex {.position = corner.outerHitOut, .texcoord = {}, .color = {1.0f, 1.0f, 1.0f, 1.0f}});
                    scope.push(vertexBase + 0);
                    scope.push(vertexBase + 1);
                    scope.push(vertexBase + 2);
                }
            }
        }

    private:
        struct StrokeCorner
        {
            float2 origin;      // Origin point where two segments meet
            float2 innerHit;    // Intersection point of the inner edges of the stroke.
            float2 outerHitIn;  // Intersection point of the outer edges of the stroke, calculated using the incoming segment.
            float2 outerHitOut; // Intersection point of the outer edges of the stroke, calculated using the outgoing segment.
        };

        StrokeCorner stroke_corner_create(const float2& prev, const float2& curr, const float2& next, float strokeWeight, float miterLimit)
        {
            const float2 dirIn = normalized(curr - prev);
            const float2 dirOut = normalized(next - curr);
            const float2 normalIn = perp(dirIn);
            const float2 normalOut = perp(dirOut);
            const float2 halfStrokeIn = normalIn * (strokeWeight / 2.0f);
            const float2 halfStrokeOut = normalOut * (strokeWeight / 2.0f);

            return StrokeCorner {
                .origin = curr,
                .innerHit = curr + halfStrokeIn + halfStrokeOut, // Intersection of inner edges
                .outerHitIn = curr - halfStrokeIn,               // Outer edge based on incoming segment
                .outerHitOut = curr - halfStrokeOut,             // Outer edge based on outgoing segment
            };
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
