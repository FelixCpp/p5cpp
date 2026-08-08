#include <p5cpp/p5cpp.hpp>

#include <iostream>

using namespace p5;

struct HelloWorldSketch : public Sketch
{
    void setup() override
    {
        std::cout << "Hello, World!" << std::endl;
    }

    void draw() override
    {
        // Drawing code goes here
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<HelloWorldSketch>();
}
