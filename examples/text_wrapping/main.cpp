#include <p5cpp.hpp>

using namespace p5cpp;

struct TextWrappingSketch : p5cpp::Sketch
{
    void setup() override
    {
        setWindowSize(800, 800);
    }

    void draw() override
    {
        background(30);

        {
            rect2f box = {100.0f, 50.0f, 600.0f, 200.0f};
            noFill();
            stroke(255);
            strokeWeight(2.0f);
            rect(box.left, box.top, box.width, box.height);

            fill(255);
            textSize(24.0f);
            textAlign(TextAlign::topLeft);
            textWrap(TextWrap::none);
            text("This is a long line of text that should not wrap and will overflow the box.", box.left, box.top, box.width);
        }

        {
            rect2f box = {100.0f, 300.0f, 600.0f, 200.0f};
            // rect2f box = {100.0f, 350.0f, getMouseX() - 100.0f, 200.0f};
            noFill();
            stroke(255);
            strokeWeight(2.0f);
            rect(box.left, box.top, box.width, box.height);

            fill(255);
            textSize(24.0f);
            textAlign(TextAlign::topLeft);
            textWrap(TextWrap::word);
            text("This is a long line of text that should wrap at word boundaries to fit within the box.", box.left, box.top, box.width);
        }

        {
            rect2f box = {100.0f, 550.0f, 600.0f, 200.0f};
            noFill();
            stroke(255);
            strokeWeight(2.0f);
            rect(box.left, box.top, box.width, box.height);

            fill(255);
            textSize(24.0f);
            textAlign(TextAlign::topLeft);
            textWrap(TextWrap::character);
            text("This is a long line of text that should wrap at character boundaries to fit within the box.", box.left, box.top, box.width);
        }
    }
};

namespace p5cpp
{
    std::unique_ptr<p5cpp::Sketch> createSketch()
    {
        return std::make_unique<TextWrappingSketch>();
    }
} // namespace p5cpp
