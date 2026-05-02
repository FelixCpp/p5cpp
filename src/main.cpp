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

        // strokeJoin(StrokeJoin::miter);
        // strokeWeight(15.0f);
        // stroke(255, 0, 0);
        // fill(0, 255, 0);
        // triangle(points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);

        // for (size_t i = 0; i < points.size(); ++i) {
        //     const float2& p1 = points[i];
        //     const float2& p2 = points[(i + 1) % points.size()];
        //
        //     // stroke(255, 128);
        //     strokeJoin(StrokeJoin::bevel);
        //     strokeCap(StrokeCap::round);
        //     stroke(255, 0, 0);
        //     fill(0, 255, 0, 100);
        //     strokeWeight(6.0f);
        //     line(p1.x, p1.y, p2.x, p2.y);
        // }

        // strokeJoin(StrokeJoin::round);
        // noFill();
        // strokeWeight(25.0f);
        // stroke(255);
        // beginShape();
        //
        // for (auto& p : points)
        //     vertex(p.x, p.y);
        //
        // endShape();
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
