#include "p5.hpp"

#include <memory>

using namespace p5;

struct Starfield : p5::Sketch
{
public:
    std::shared_ptr<Font> font;
    std::shared_ptr<Canvas> canvas;
    std::shared_ptr<Canvas> cv;

    void setup() override
    {
        font = loadFont("Skia.ttf");
        canvas = createCanvas(200, 200);
        cv = createCanvas(200, 200);

        pushCanvas(cv);
        background(0);
        rect(50.0f, 50.0f, 100.0f, 100.0f);
        // textSize(42.0f);
        // text("Hello", 100.0f, 100.0f);
        popCanvas();
    }

    void draw() override
    {
        background(51);

        pushCanvas(canvas);
        background(0, 0, 255);
        popCanvas();

        image(cv->getTextureId(), 100.0f, 100.0f, 300.0f, 300.0f);
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
