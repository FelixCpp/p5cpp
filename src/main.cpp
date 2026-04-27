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
        background(21, 21, 21);
        fill(255, 0, 0);
        stroke(0, 255, 0);
        strokeWeight(15.0f);
        strokeCap(StrokeCap::round);
        rect(100.0f, 100.0f, 300.0f, 300.0f);

        // strokeCap(StrokeCap::round);
        // for (int i = 1; i < points.size(); i += 2) {
        //     strokeWeight(15.0f);
        //     stroke(255, 0, 255);
        //     line(points[i - 1].x, points[i - 1].y, points[i].x, points[i].y);
        //
        //     stroke(0, 255, 0);
        //     point(points[i - 1].x, points[i - 1].y);
        //     point(points[i].x, points[i].y);
        // }
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
