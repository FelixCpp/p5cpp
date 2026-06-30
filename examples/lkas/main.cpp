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
        rect(100, 100, 200, 200);
    }
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<LaneKeepingAssistentSimulation>();
    }
} // namespace p5cpp
