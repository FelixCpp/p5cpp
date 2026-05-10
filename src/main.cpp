#include "p5.hpp"

#include <memory>

using namespace p5;

struct Starfield : p5::Sketch
{
public:
    std::shared_ptr<Canvas> layer;
    // std::shared_ptr<Canvas> layer2;

    void setup() override
    {
        layer = createCanvas(400, 400);
        // layer2 = createCanvas(200, 200);

        pushCanvas(layer);
        background(100);
        popCanvas();
    }

    void draw() override
    {
        background(51);
        // fill(255, 0, 0);
        // rect(100.0f, 100.0f, 200.0f, 200.0f);

        // textSize(48.0f);
        // fill(255);
        // text("Hello, World!", 150.0f, 150.0f);

        // canvas(layer);
        // background(255);
        // fill(255, 0, 0);
        // rect(50.0f, 50.0f, 150.0f, 150.0f);
        // fill(0, 255, 0);
        // rect(200.0f, 50.0f, 150.0f, 150.0f);
        // noCanvas();

        // image(layer2->getTextureId(), 50.0f, 50.0f, 150.0f, 150.0f);
        // background(255, 0, 0);
        image(layer->getTextureId(), 150.0f, mouseY, 400.0f, 400.0f);
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
