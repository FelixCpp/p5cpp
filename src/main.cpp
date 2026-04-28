#include "p5.hpp"

#include <vector>
#include <memory>
#include <array>

using namespace p5;

struct Shape
{
    virtual ~Shape() = default;
    virtual void begin() = 0;
    virtual void end() = 0;
    virtual void vertex(float x, float y, color_t col = color(255, 255, 255)) = 0;
    virtual void arc(float2 center, float startAngle, float sweepAngle, float radius, bool clockwise) = 0;
    virtual void quad(float2 a, float2 b, float2 c, float2 d) = 0;
};

// struct PolygonShape : Shape
// {
// };

static std::array<color_t, 100> getRandomColors()
{
    std::array<color_t, 100> colors;
    srand(time(nullptr));
    for (size_t i = 0; i < colors.size(); ++i) {
        colors[i] = color(rand() % 256, rand() % 256, rand() % 256);
    }
    return colors;
}

inline static std::array<color_t, 100> randomColors = getRandomColors();
static int colorIndex = 0;

static color_t getRandomColor()
{
    return randomColors[colorIndex++ % randomColors.size()];
}

struct PointedShape : Shape
{
    virtual void begin() override
    {
        colorIndex = 0;
        strokeWeight(5.0f);
        stroke(255);
    }

    virtual void end() override {}
    virtual void vertex(float x, float y, color_t color) override
    {
        stroke(getRandomColor());
        point(x, y);
    }

    virtual void arc(float2 center, float startAngle, float sweepAngle, float radius, bool clockwise) override
    {
        const size_t segments = 16;
        const float angleStep = sweepAngle / segments;

        std::vector<float2> pts;

        for (size_t i = 0; i <= segments; ++i) {
            float t = static_cast<float>(i) / segments;
            float adjustedT = clockwise ? t : (1.0f - t);
            float angle = startAngle + adjustedT * sweepAngle;
            float px = center.x + std::cos(angle) * radius;
            float py = center.y + std::sin(angle) * radius;
            pts.push_back(float2 {px, py});
        }

        for (size_t i = 0; i < pts.size() - 1; ++i) {
            float2 a = pts[i];
            float2 b = pts[i + 1];
            fill(getRandomColor());
            triangle(center.x, center.y, a.x, a.y, b.x, b.y);
        }
    }

    virtual void quad(float2 a, float2 b, float2 c, float2 d) override
    {
        // fill(getRandomColor());
        // triangle(a.x, a.y, b.x, b.y, c.x, c.y);
        // fill(getRandomColor());
        // triangle(a.x, a.y, c.x, c.y, d.x, d.y);

        stroke(255);
        strokeWeight(5.0f);
        line(a.x, a.y, b.x, b.y);
        line(b.x, b.y, c.x, c.y);
        line(c.x, c.y, d.x, d.y);
        line(d.x, d.y, a.x, a.y);
    }
};

struct DemoSketch : p5::Sketch
{
public:
    std::unique_ptr<Shape> shape;

    void setup() override
    {
        shape = std::make_unique<PointedShape>();
    }

    void draw() override
    {
        colorIndex = 0;
        background(21, 21, 21);

        float thickness = 35.0f;
        constexpr int s = 3;
        const float2 pts[s] = {
            float2 {100.0f, 100.0f},
            float2 {300.0f, 200.0f},
            float2 {(float)mouseX, (float)mouseY},
        };

        shape->begin();

        // Append start cap
        for (int i = 0; i < s - 1; ++i) {
            float2 currPt = pts[i];
            float2 nextPt = pts[i + 1];
            float2 dir = normalized(nextPt - currPt);
            float2 normal = perp(dir);

            shape->quad(
                currPt + normal * thickness,
                currPt - normal * thickness,
                nextPt - normal * thickness,
                nextPt + normal * thickness
            );
        }

        for (size_t i = 1; i < s - 1; ++i) {
            float2 prevPt = pts[i - 1];
            float2 currPt = pts[i];
            float2 nextPt = pts[i + 1];

            float2 dirA = normalized(currPt - prevPt);
            float2 dirB = normalized(nextPt - currPt);
            float2 normalA = perp(dirA);
            float2 normalB = perp(dirB);

            StrokeJoin join = StrokeJoin::miter;
            switch (join) {
                case StrokeJoin::miter: {
                    float2 bisector = normalized(normalA + normalB);
                    float miterLength = thickness / dot(bisector, normalA);

                    float2 miterTop = currPt + bisector * miterLength; // eine Seite der Spitze
                    float2 miterBot = currPt - bisector * miterLength; // andere Seite

                    shape->quad(
                        currPt + normalA * thickness, // Einlaufkante, oben
                        currPt - normalA * thickness, // Einlaufkante, unten
                        miterBot,                     // Miter-Spitze, unten
                        miterTop                      // Miter-Spitze, oben
                    );
                } break;

                case StrokeJoin::bevel: {
                    shape->vertex(currPt.x + normalA.x * thickness, currPt.y + normalA.y * thickness);
                    shape->vertex(currPt.x - normalA.x * thickness, currPt.y - normalA.y * thickness);
                    shape->vertex(currPt.x - normalB.x * thickness, currPt.y - normalB.y * thickness);
                    shape->vertex(currPt.x + normalB.x * thickness, currPt.y + normalB.y * thickness);

                    shape->quad(
                        currPt + normalA * thickness,
                        currPt - normalA * thickness,
                        currPt - normalB * thickness,
                        currPt + normalB * thickness
                    );
                } break;

                case StrokeJoin::round: {
                    float angleA = std::atan2(dirA.y, dirA.x) + M_PI * 0.5f;
                    float angleB = std::atan2(dirB.y, dirB.x) + M_PI * 0.5f;
                    shape->arc(currPt, angleA, angleB - angleA, thickness, true);
                } break;
            }
        }

        const StrokeCap cap = StrokeCap::square;
        { // Start Cap

            switch (cap) {
                case StrokeCap::butt: break;

                case StrokeCap::square: {
                    float2 dir = normalized(pts[1] - pts[0]);
                    float2 normal = perp(dir);
                    float2 offset = dir * thickness * 0.5f;

                    shape->quad(
                        pts[0] + normal * thickness + offset,
                        pts[0] - normal * thickness + offset,
                        pts[0] - normal * thickness - offset,
                        pts[0] + normal * thickness - offset
                    );
                } break;

                case StrokeCap::round: {
                    float2 dir = normalized(pts[1] - pts[0]);
                    float2 normal = perp(dir);
                    float angle = M_PI * 0.5f + std::atan2(dir.y, dir.x);
                    shape->arc(pts[0], angle, M_PI, thickness, true);
                } break;
            }
        }
        { // End Cap
            switch (cap) {
                case StrokeCap::butt: break;
                case StrokeCap::square: {
                    float2 dir = normalized(pts[s - 1] - pts[s - 2]);
                    float2 offset = dir * thickness * 0.5f;
                    float2 normal = perp(dir);
                    shape->quad(
                        pts[s - 1] + normal * thickness + offset,
                        pts[s - 1] - normal * thickness + offset,
                        pts[s - 1] - normal * thickness - offset,
                        pts[s - 1] + normal * thickness - offset
                    );
                } break;
                case StrokeCap::round: {
                    float2 dir = normalized(pts[s - 1] - pts[s - 2]);
                    float2 normal = perp(dir);
                    float angle = -M_PI * 0.5f + std::atan2(dir.y, dir.x);
                    shape->arc(pts[s - 1], angle, M_PI, thickness, true);
                } break;
            }
        }
        shape->end();
    }

    void destroy() override
    {
    }

    void mousePressed(int mx, int my) override
    {
    }
};

namespace p5
{
    std::unique_ptr<p5::Sketch> createSketch()
    {
        return std::make_unique<DemoSketch>();
    }
} // namespace p5
