#include "p5.hpp"

using namespace p5;

struct DemoSketch : p5::Sketch
{
public:
    void setup() override
    {
    }

    void draw() override
    {
        background(21, 21, 21);
        fill(255, 0, 0);
        // rect(100.0f, 100.0f, 300.0f, 300.0f);
        ellipse(400.0f, 300.0f, 50.0f, 50.0f);

        fill(0, 255, 0);
    }

    void destroy() override
    {
    }
};

namespace p5
{
    std::unique_ptr<p5::Sketch> createSketch()
    {
        return std::make_unique<DemoSketch>();
    }
} // namespace p5
