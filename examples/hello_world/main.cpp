#include <p5cpp/p5cpp.hpp>

#include <cstdlib>
#include <ctime>

using namespace p5;

struct HelloWorldSketch : public Sketch
{
    float currentStrokeWeight = 0.0f;
    void setup() override
    {
        setWindowSize(800, 600);
        background(rgba(255));
    }

    void draw() override
    {
        if (isKeyPressed(Key::C)) {
            background(rgba(255));
        }

        if (isMouseButtonDown(MouseButton::Left)) {
            const float mouseDeltaX = static_cast<float>(getMouseDeltaX());
            const float mouseDeltaY = static_cast<float>(getMouseDeltaY());
            const float strokeThickness = std::sqrt(mouseDeltaX * mouseDeltaX + mouseDeltaY * mouseDeltaY);
            currentStrokeWeight = lerp(currentStrokeWeight, strokeThickness, 0.1f);
            strokeWeight(currentStrokeWeight);
            stroke(rgba(0, 0, 0, 255));
            strokeCap(StrokeCap::round);
            line(getMouseX(), getMouseY(), getPreviousMouseX(), getPreviousMouseY());
        }
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<HelloWorldSketch>();
}
