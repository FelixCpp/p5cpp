#include "p5.hpp"

#include <memory>

using namespace p5;

struct Starfield : p5::Sketch
{
public:
    std::shared_ptr<Font> font;
    std::shared_ptr<Canvas> redRect;
    std::shared_ptr<Canvas> blueRect;

    void setup() override
    {
        font = loadFont("Skia.ttf");
        redRect = createCanvas(200, 200);
        blueRect = createCanvas(200, 200);
    }

    void draw() override
    {
        background(51);
        noStroke();
        fill(255);
        rect(10.0f, 10.0f, 200.0f, 200.0f);
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
