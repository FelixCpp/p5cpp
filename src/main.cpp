#include "p5.hpp"

#include <vector>
#include <memory>

using namespace p5;

struct DemoSketch : p5::Sketch
{
public:
    void setup() override
    {
    }

    void draw() override
    {
        const std::vector<float2> points = {
            {100.0f + 100.0f, 100.0f},
            {(float)mouseX, (float)mouseY},
            {100.0f + 300.0f, 300.0f},
            {100.0f + 100.0f, 300.0f},
        };

        background(21, 21, 21);

        strokeJoin(StrokeJoin::round);
        noFill();
        strokeWeight(25.0f);
        stroke(255);
        beginShape();

        for (auto& p : points)
            vertex(p.x, p.y);

        endShape();

        // for (size_t i = 0; i < points.size(); ++i) {
        //     const size_t nextIndex = (i + 1) % points.size();
        //     stroke(255, 10);
        //     strokeWeight(35.0f);
        //     line(points[i].x, points[i].y, points[nextIndex].x, points[nextIndex].y);
        // }

        // drawStroke(
        //     points,
        //     35.0f,
        //     StrokeCap::round,
        //     StrokeJoin::round,
        //     StrokeAlign::center,
        //     10.0f,
        //     true
        // );

        // arc(400.0f, 300.0f, 200.0f, 200.0f, 0.0f, M_PI);
    }

    void destroy() override
    {
    }

    void drawStroke(const std::vector<float2>& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, StrokeAlign strokeAlign, float miterLimit, bool close)
    {
        const size_t n = points.size();
        if (n < 2) return;

        const size_t segmentCount = close ? n : n - 1;
        const float half = strokeWeight * 0.5f;

        for (size_t i = 0; i < segmentCount; ++i) {
            const bool isFirst = (i == 0);
            const bool isLast = (i == segmentCount - 1);

            const float2& prevPos = points[(i + n - 1) % n];
            const float2& currPos = points[i];
            const float2& nextPos = points[(i + 1) % n];
            const float2& afterPos = points[(i + 2) % n]; // für Corner am Ende

            const float2 dir = normalized(nextPos - currPos);
            const float2 normal = perp(dir);

            // Segmentgrenzen: defaults (werden für Caps / bei geradem Pfad benutzt)
            float2 segStartOuter = currPos + normal * half;
            float2 segStartInner = currPos - normal * half;
            float2 segEndOuter = nextPos + normal * half;
            float2 segEndInner = nextPos - normal * half;

            fill(255, 255);
            triangle(segStartOuter.x, segStartOuter.y, segStartInner.x, segStartInner.y, segEndInner.x, segEndInner.y);
            triangle(segStartOuter.x, segStartOuter.y, segEndInner.x, segEndInner.y, segEndOuter.x, segEndOuter.y);

            StrokeCorner sc = stroke_corner_create(prevPos, currPos, nextPos, strokeWeight, miterLimit);
            // fill(255, 0, 0);
            // circle(sc.origin.x, sc.origin.y, 10.0f);
            // fill(0, 255, 0);
            // circle(sc.innerIntersection.x, sc.innerIntersection.y, 10.0f);
            // fill(0, 100, 255);
            // circle(sc.outerIntersection.x, sc.outerIntersection.y, 10.0f);
            // fill(100, 0, 255);
            // circle(sc.joinStart.x, sc.joinStart.y, 10.0f);
            // fill(100, 255, 255);
            // circle(sc.joinEnd.x, sc.joinEnd.y, 10.0f);

            fill(255);

            // MITER
            // triangle(sc.outerIntersection.x, sc.outerIntersection.y, sc.joinStart.x, sc.joinStart.y, sc.joinEnd.x, sc.joinEnd.y);
            // triangle(sc.origin.x, sc.origin.y, sc.joinStart.x, sc.joinStart.y, sc.joinEnd.x, sc.joinEnd.y);

            // BEVEL
            // triangle(sc.origin.x, sc.origin.y, sc.joinStart.x, sc.joinStart.y, sc.joinEnd.x, sc.joinEnd.y);

            // ROUND
            fill(255, 0, 0);
            circle(currPos.x, currPos.y, strokeWeight);
            // arc(
            //     currPos.x,
            //     currPos.y,
            //     strokeWeight,
            //     strokeWeight,
            //     std::atan2(segStartInner.y - currPos.y, segStartInner.x - currPos.x),
            //     std::atan2(segStartOuter.y - currPos.y, segStartOuter.x - currPos.x)
            // );
        }
    }

    void arc(float centerX, float centerY, float width, float height, float startAngle, float stopAngle)
    {
        beginShape();

        constexpr size_t segmentCount = 32;
        const float sweepAngle = std::abs(stopAngle - startAngle);

        for (size_t i = 0; i <= segmentCount; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(segmentCount);
            float angle = startAngle + t * sweepAngle;
            float x = centerX + std::cos(angle) * width * 0.5f;
            float y = centerY + std::sin(angle) * height * 0.5f;
            // vertex(centerX, centerY);
            vertex(x, y);
        }

        endShape();
    }

    struct StrokeCorner
    {
        float2 origin;            // P  — der originale Scheitelpunkt
        float2 innerIntersection; // M  — Schnittpunkt der inneren Kanten
        float2 outerIntersection; // Schnittpunkt der äußeren Kanten (Miter-Punkt)
        float2 joinStart;         // bR1 — Endpunkt der äußeren Kante des eingehenden Segments
        float2 joinEnd;           // aR1 — Startpunkt der äußeren Kante des ausgehenden Segments
    };

    StrokeCorner stroke_corner_create(const float2& prev, const float2& curr, const float2& next, float strokeWeight, float miterLimit)
    {
        const float half = strokeWeight * 0.5f;

        float2 dIn = normalized(curr - prev);
        float2 dOut = normalized(next - curr);
        float2 nIn = perp(dIn);
        float2 nOut = perp(dOut);

        float crossVal = p5::cross(dIn, dOut);
        bool leftTurn = (crossVal >= 0.0f);
        float outerSign = leftTurn ? -1.0f : 1.0f;
        float innerSign = -outerSign;

        // joinStart (bR1): Eckpunkt der äußeren Kante, eingehendes Segment
        float2 joinStart = curr + nIn * (half * outerSign);

        // joinEnd (aR1): Eckpunkt der äußeren Kante, ausgehendes Segment
        float2 joinEnd = curr + nOut * (half * outerSign);

        // innerIntersection (M): Schnittpunkt der inneren Kanten
        float2 innerBase = curr + nIn * (half * innerSign);
        float2 innerIntersection;
        if (!line_intersect(curr + nIn * (half * innerSign), dIn, curr + nOut * (half * innerSign), dOut, innerIntersection))
            innerIntersection = curr + nIn * (half * innerSign); // Fallback

        // outerIntersection: Schnittpunkt der äußeren Kanten (Miter-Punkt, N in der Grafik)
        float2 outerIntersection;
        if (!line_intersect(joinStart, dIn, joinEnd, dOut, outerIntersection))
            outerIntersection = joinStart; // Fallback bei parallelen Segmenten

        // Miter-Limit: wenn zu weit, outerIntersection auf origin setzen
        // (Join-Code fällt dann auf Bevel zurück)
        if (length(outerIntersection - curr) > miterLimit * half)
            outerIntersection = curr; // Signal: Miter ungültig

        return StrokeCorner {
            .origin = curr,
            .innerIntersection = innerIntersection,
            .outerIntersection = outerIntersection,
            .joinStart = joinStart,
            .joinEnd = joinEnd,
        };
    }

    // --- Schnittpunkt zweier Geraden (parametrisch) ---
    // Gerade A: P + t*D_a  ,  Gerade B: Q + s*D_b
    // t = cross2(Q-P, D_b) / cross2(D_a, D_b)
    inline static auto line_intersect = [](float2 P, float2 Da,
                                           float2 Q, float2 Db,
                                           float2& hit) -> bool {
        float denom = cross(Da, Db);
        if (std::abs(denom) < 1e-6f) return false; // parallel
        float t = cross(Q - P, Db) / denom;
        hit = P + Da * t;
        return true;
    };

    std::vector<float2> points;

    void mousePressed(int mx, int my) override
    {
        points.push_back(float2 {.x = (float)mx, .y = (float)my});
    }
};

namespace p5
{
    std::unique_ptr<p5::Sketch> createSketch()
    {
        return std::make_unique<DemoSketch>();
    }
} // namespace p5
