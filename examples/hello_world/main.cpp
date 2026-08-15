#include <p5cpp/p5cpp.hpp>

using namespace p5;

struct EffectsSketch : public Sketch
{
    void setup() override
    {
        setWindowSize(800, 600);
    }

    void draw() override
    {
        background(rgba(25));
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<EffectsSketch>();
}
