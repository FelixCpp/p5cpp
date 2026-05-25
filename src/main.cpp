#include "p5.hpp"

#include <memory>

using namespace p5;

struct Starfield : p5::Sketch
{
public:
    std::shared_ptr<Font> skia;
    std::shared_ptr<Texture> texture;

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
