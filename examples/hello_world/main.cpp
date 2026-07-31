#include <p5cpp/p5cpp.hpp>
using namespace p5;

struct HelloWorldSketch : Sketch
{
    void setup() override
    {
        setWindowSize(800, 600);
        setWindowTitle("Hello World");

        info("Test {}", 123);
    }

    void draw() override
    {
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<HelloWorldSketch>();
}
