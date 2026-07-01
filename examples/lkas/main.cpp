#include <p5cpp.hpp>

using namespace p5cpp;

struct LaneKeepingAssistentSimulation : Sketch
{
    void setup() override
    {
    }

    void draw() override
    {
        background(21);
        rect(0.0f, 0.0f, 100.0f, 100.0f);
        strokeWeight(15.0f);
        stroke(255, 0, 0);
        circle(getMouseX(), getMouseY(), 50.0f);
    }
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<LaneKeepingAssistentSimulation>();
    }
} // namespace p5cpp
