#include <algorithm>
#include <p5cpp.hpp>

using namespace p5cpp;

struct Demo : Sketch
{
    float time = 0.0f;
    std::vector<float> wave;
    bool isMousePressed = false;

    void setup() override
    {
        setWindowSize(600, 400);
        frameRate(60);
    }

    void event(const WindowEvent& event) override
    {
        if (event.type == EventType::mousePress) {
            isMousePressed = true;
        } else if (event.type == EventType::mouseRelease) {
            isMousePressed = false;
        }
    }

    void draw() override
    {
        const float width = static_cast<float>(getWidth());
        const float height = static_cast<float>(getHeight());

        background(0);
        translate(width * 0.25f, height * 0.5f);

        float x = 0.0f;
        float y = 0.0f;

        const float sliderValue = drawSlider(10.0f, height - 30.0f, width - 20.0f, 1, 100, 5);
        const size_t depth = static_cast<size_t>(sliderValue);

        for (size_t i = 0; i < depth; ++i) {
            float prevx = x;
            float prevy = y;

            const size_t n = i * 2 + 1;
            const float radius = 75.0f * (4.0f / (static_cast<float>(n) * PI));

            x += radius * std::cos(n * time);
            y += radius * std::sin(n * time);

            stroke(255, 100);
            noFill();
            circle(prevx, prevy, radius * 2);

            // fill(255);
            stroke(255);
            line(prevx, prevy, x, y);
            // circle(x, y, 8.0f);

            // translate(200, 0);
            // line(x - 200, y, 0, wave.at(0));
        }

        wave.insert(wave.begin(), y);

        translate(200.0f, 0.0f);
        stroke(255);
        line(x - 200.0f, y, 0.0f, wave.at(0));

        beginShape();
        noFill();
        for (size_t i = 0; i < wave.size(); ++i) {
            vertex(i, wave.at(i));
        }
        endShape(ShapeType::polygon, false);

        if (wave.size() > 250) {
            wave.pop_back();
        }

        time += 0.05f;
    }

    float drawSlider(float left, float y, float width, float min, float max, float initialValue)
    {
        pushMatrix();
        resetMatrix();

        static float value = initialValue;
        const float mouseX = static_cast<float>(getMouseX());
        const float mouseY = static_cast<float>(getMouseY());

        stroke(255);
        line(left, y, left + width, y);

        const float handleX = left + (value - min) / (max - min) * width;
        const float handleRadius = 10.0f;

        const bool isHovering = (mouseX - handleX) * (mouseX - handleX) + (mouseY - y) * (mouseY - y) <= handleRadius * handleRadius;
        const color_t handleColor = isHovering || isMousePressed ? 0xFFFFFFFF : 0xFFFFFF80;

        if (isHovering and isMousePressed) {
            const float clampedMouseX = std::clamp(mouseX, left, left + width);
            value = min + (clampedMouseX - left) / width * (max - min);
        }

        fill(handleColor);
        circle(handleX, y, handleRadius * 2);

        textAlign(TextAlign::bottomLeft);
        textSize(12.0f);
        fill(255);
        text("Depth: " + std::to_string(static_cast<int>(value)), left, y - handleRadius - 5.0f);

        popMatrix();
        return value;
    }
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<Demo>();
    }
} // namespace p5cpp
