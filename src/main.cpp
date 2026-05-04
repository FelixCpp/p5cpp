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

    float angle = 0.0f;

    void draw() override
    {
        angle += 0.01f;

        curveDetail(50);
        strokeWeight(5.0f);
        background(21);

        noFill();

        if (not points.empty()) {
            beginShape();
            curveVertex(points[0].x, points[0].y);
            for (size_t i = 0; i < points.size(); ++i) {
                stroke(255, 102, 0);
                strokeWeight(5.0f);
                curveVertex(points[i].x, points[i].y);
            }
            curveVertex(points[points.size() - 1].x, points[points.size() - 1].y);
            endShape();
        }

        for (size_t i = 0; i < points.size(); ++i) {
            stroke(255);
            point(points[i].x, points[i].y);
        }
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
