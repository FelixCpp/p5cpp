#include <p5cpp.hpp>

using namespace p5cpp;

struct LaneKeepingAssistentSimulation : Sketch
{
    void setup() override
    {
    }

    void draw() override
    {
    }
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<LaneKeepingAssistentSimulation>();
    }
} // namespace p5cpp
