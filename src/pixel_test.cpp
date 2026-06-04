#include "p5.hpp"

using namespace p5;

struct PixelTestSketch : Sketch
{

    void setup() override
    {
    }

    void draw() override
    {
        Pixels pixels = loadPixels();
        for (uint32_t y = 0; y < pixels.height; ++y) {
            for (uint32_t x = 0; x < pixels.width; ++x) {
                int red = random(255);
                int green = random(255);
                int blue = random(255);
                pixels[y * width + x] = rgba(red, green, blue);
            }
        }
        updatePixels(pixels);
    }
};

namespace p5
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<PixelTestSketch>();
    }
} // namespace p5
