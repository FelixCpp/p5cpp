#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>
using namespace p5;

struct InteractiveText : Sketch
{
    std::shared_ptr<Font> font = loadFontFromFile("Arial Rounded Bold.ttf");

    void setup() override
    {
        rect2f bounds = textBounds(*font, 320.0f, "BIGGER");
        setWindowSize(bounds.width, bounds.height);
    }

    void draw() override
    {
        background(rgba(255));

        fill(rgba(0));
        noStroke();
        textFont(font);
        textSize(320.0f);
        textAlign(TextAlignment::center);
        text("BIGGER", getWidth() * 0.5f, getHeight() * 0.5f, getWidth(), getHeight());
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<InteractiveText>();
}
