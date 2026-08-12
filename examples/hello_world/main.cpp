#include <p5cpp/p5cpp.hpp>

using namespace p5;

struct HelloWorldSketch : public Sketch
{
    std::shared_ptr<Framebuffer> framebuffer = createFramebuffer(200, 200);

    void setup() override
    {
        withFramebuffer(framebuffer, []() {
            fill(rgba(255, 0, 0, 255));
            stroke(rgba(0, 255, 0, 255));
            strokeWeight(5.0f);
            rect(50.0f, 50.0f, 100.0f, 100.0f);
        });
    }

    void draw() override
    {
        background(rgba(255, 0, 0, 255));
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<HelloWorldSketch>();
}
