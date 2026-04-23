#include "p5.hpp"

#include <vector>

using namespace p5;

struct DemoSketch : p5::Sketch
{
public:
    std::vector<float2> points;

    void setup() override
    {
    }

    void draw() override
    {
        background(255, 255, 255);

        // fill(255, 0, 0);
        // rect(100.0f, 100.0f, 200.0f, 200.0f, 25.0f, 15.0f);

        // fill(0, 255, 0);
        // ellipse(600.0f, 200.0f, 200.0f, 200.0f);

        // fill(0, 0, 255);
        // triangle(100.0f, 400.0f, 100.0f, 500.0f, 300.0f, 450.0f);

        fill(255, 0, 0);
        beginShape();
        for (auto& p : points) vertex(p.x, p.y);
        endShape();
    }

    void destroy() override
    {
    }

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
