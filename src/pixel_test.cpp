#include "p5.hpp"

using namespace p5;

struct PixelTestSketch : Sketch
{

    void setup() override
    {
    }

    void draw() override
    {
        Pixels px = loadPixels();
        for (int y = 0; y < px.height; ++y) {
            for (int x = 0; x < px.width; ++x) {
                // px[y * px.width + x] = rgba(x % 256, y % 256, 128, 255);
                // int r = random(256);
                // int g = random(256);
                // int b = random(256);
                // px[y * px.width + x] = rgba(r, g, b, 255);
            }
        }
        updatePixels(px);
    }
};

namespace p5
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<PixelTestSketch>();
    }
} // namespace p5
