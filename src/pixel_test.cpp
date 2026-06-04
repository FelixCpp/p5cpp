#include "p5.hpp"

using namespace p5;

struct PixelTestSketch : Sketch
{

    void setup() override
    {
    }

    void draw() override
    {
        background(255);
    }
};

namespace p5
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<PixelTestSketch>();
    }
} // namespace p5
