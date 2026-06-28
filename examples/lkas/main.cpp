#include <p5cpp.hpp>

using namespace p5cpp;

void drawRoad(std::span<const float2> points, float width)
{
    std::vector<float2> leftSide;
    std::vector<float2> rightSide;

    for (size_t i = 0; i < points.size(); ++i) {
        const float2& p1 = points[i];
        const float2& p2 = points[(i + 1) % points.size()];

        float2 dir = normalized(p2 - p1);
        float2 normal = float2(-dir.y, dir.x);

        leftSide.emplace_back(p1 + normal * (width / 2.0f));
        rightSide.emplace_back(p1 - normal * (width / 2.0f));
    }

    noStroke();
    fill(100);
    beginShape();
    for (size_t i = 0; i < leftSide.size(); i++) {
        const float2& p1 = leftSide[i];
        const float2& p2 = rightSide[i];
        const float2& p3 = leftSide[(i + 1) % leftSide.size()];
        const float2& p4 = rightSide[(i + 1) % rightSide.size()];

        vertex(p1.x, p1.y);
        vertex(p2.x, p2.y);
    }
    endShape(ShapeType::triangleStrip, false);
}

struct RoadBuilder
{
    std::vector<float2> points;

    void placePoint(const float2& point)
    {
        points.emplace_back(point);
    }

    void show()
    {
        for (size_t i = 0; i < points.size(); ++i) {
            const float2& p1 = points[i];

            stroke(255);
            strokeWeight(5.0f);
            point(p1.x, p1.y);
        }

        drawRoad(points, 40.0f);
    }
};

struct LaneKeepingAssistentSimulation : Sketch
{
    RoadBuilder roadBuilder;

    void setup() override
    {
        setWindowSize(800, 800);
        setWindowResizable(false);
        setWindowTitle("Lane Keeping Assistent Simulation");
    }

    void event(const WindowEvent& e) override
    {
        if (e.type == EventType::mousePress && e.mouseButton.button == MouseButton::left) {
            roadBuilder.placePoint(float2(e.mouseButton.x, e.mouseButton.y));
        }
    }

    void draw() override
    {
        background(21);
        roadBuilder.show();
    }
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<LaneKeepingAssistentSimulation>();
    }
} // namespace p5cpp
