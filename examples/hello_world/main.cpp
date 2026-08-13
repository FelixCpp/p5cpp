#include <p5cpp/p5cpp.hpp>

#include <cstdlib>
#include <ctime>

using namespace p5;

struct EffectsSketch : public Sketch
{
    void setup() override
    {
        setWindowSize(800, 600);
    }

    void draw() override
    {
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<EffectsSketch>();
}
