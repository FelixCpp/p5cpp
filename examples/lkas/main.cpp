#include <p5cpp/p5cpp.hpp>
#include <format>

using namespace p5cpp;

struct InputSketch : Sketch
{
    bool paused = false;

    void setup() override { setWindowSize(800, 600); }

    void draw() override
    {
        background(40);
        if (!paused) {
            fill(100, 200, 255);
            noStroke();
            circle((float)getMouseX(), (float)getMouseY(), 60);
        }
    }

    void event(const WindowEvent& e) override
    {
        if (e.type == EventType::keyPress && e.keyEvent.key == Key::space)
            paused = !paused;

        if (e.type == EventType::mousePress && e.mouseButton.button == MouseButton::left)
            info(std::format("click at {}, {}", getMouseX(), getMouseY()));

        if (e.type == EventType::keyPress && e.keyEvent.key == Key::escape)
            quit();
    }
};

std::unique_ptr<Sketch> p5cpp::createSketch()
{
    return std::make_unique<InputSketch>();
}
