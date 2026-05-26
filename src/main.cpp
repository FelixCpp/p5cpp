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
        textFont(skia);
        textSize(64.0f);
        text("Hello, p5!", 100.0f, 200.0f);

        fill(255, 0, 0);
        stroke(255);
        strokeWeight(4.0f);
        beginShape();
        vertex(100.0f, 100.0f);
        vertex(100.0f, 300.0f);
        vertex(300.0f, 300.0f);
        vertex(300.0f, 100.0f);

        vertex(500.0f, 100.0f);
        vertex(500.0f, 300.0f);
        endShape(ShapeType::quadStrip);
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
