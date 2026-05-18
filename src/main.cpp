#include "p5.hpp"

#include <memory>

using namespace p5;

struct Starfield : p5::Sketch
{
public:
    std::shared_ptr<Font> font;
    std::shared_ptr<Framebuffer> blurCanvas = createCanvas(800, 600);

    void setup() override
    {
        font = loadFont("Skia.ttf");

        pushCanvas(blurCanvas);
        {
            fill(40, 160, 230);
            rect(100, 100, 200, 200);
            strokeWeight(15.0f);
            stroke(255, 0, 0);
            ellipse(400, 300, 100, 100);
        }
        popCanvas();
    }

    void draw() override
    {
        background(51);
        fill(255, 0, 0);

        pushCanvas(blurCanvas);
        {
            textFont(font);
            textSize(48.0f);
            fill(255);
            text("Hello, p5!", mouseX, mouseY);
        }
        popCanvas();

        image(blurCanvas->getTextureId(), 0.0f, 0.0f, 800.0f, 600.0f);
        text("Ich bin Felix!", 100, 300);
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
