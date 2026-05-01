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
        background(21, 21, 21);
        // noFill();
        // strokeWeight(25.0f);
        // stroke(255, 0, 0);
        // strokeJoin(StrokeJoin::miter);
        //
        // beginShape();
        // for (float2& p : points) vertex(p.x, p.y);
        // endShape();

        strokeJoin(StrokeJoin::miter);
        noFill();
        stroke(255);
        strokeWeight(35.0f);
        beginShape();
        vertex(100.0f, 100.0f);
        vertex(mouseX, mouseY);
        vertex(300.0f, 300.0f);
        vertex(100.0f, 300.0f);
        endShape();
        // rect(100.0f, 100.0f, 300.0f, 300.0f);
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
