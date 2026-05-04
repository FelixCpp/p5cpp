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

        stroke(255, 0, 0);
        strokeWeight(5.0f);
        fill(0, 255, 0);
        noStroke();
        arc(400.0f, 300.0f, 200.0f, 200.0f, degrees(0.0f), degrees(180.0f), ArcMode::open);
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
