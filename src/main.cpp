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

        strokeCap(StrokeCap::round);
        strokeJoin(StrokeJoin::round);
        strokeWeight(35.0f);
        stroke(255, 0, 0);
        fill(0, 255, 0);
        noFill();

        beginShape();
        for (auto& p : points)
            vertex(p.x, p.y);
        endShape();
    }

    void destroy() override
    {
    }

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
