#include "p5.hpp"

using namespace p5;

struct PixelTestSketch : Sketch
{

    void setup() override
    {
    }

    void draw() override
    {
        // background(255);
        // fill(255, 0, 0);
        // stroke(0, 255, 0);
        // strokeWeight(5.0f);
        // rect(10, 10, 100, 100);
        Pixels pixels = loadPixels();
        for (unsigned int y = 0; y < pixels.height; ++y) {
            for (unsigned int x = 0; x < pixels.width; ++x) {
                pixels[y * width + x] = rgba(255, 0, 0);
            }
        }
        updatePixels(pixels);
        // background(255);
    }
};

namespace p5
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<PixelTestSketch>();
    }
} // namespace p5
