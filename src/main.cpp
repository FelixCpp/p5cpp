#include "p5.hpp"

#include <memory>

using namespace p5;

struct Starfield : p5::Sketch
{
public:
    std::shared_ptr<Font> skia;

    void setup() override
    {
        skia = loadFont("Skia.ttf");
    }

    void draw() override
    {
        background(51);

        fill(255, 0, 0);
        stroke(255);
        strokeWeight(18.0f);
        line(200.0f, 200.0f, 300.0f, 150.0f);
    }

    void destroy() override
    {
    }

    void mousePressed(int x, int y) override
    {
    }
};

namespace p5
{
    std::unique_ptr<p5::Sketch> createSketch()
    {
        return std::make_unique<Starfield>();
    }
} // namespace p5
