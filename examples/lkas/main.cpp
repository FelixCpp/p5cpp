#include <p5cpp/p5cpp.hpp>

using namespace p5cpp;

struct ExampleModule : Module
{
    void setup(AppContext& context, Next next) override
    {
        info("ExampleModule setup");
        next();
    }

    void event(AppContext& context, WindowEvent& event, Next next) override
    {
        next();
    }

    void draw(AppContext& context, Next next) override
    {
        const float mx = static_cast<float>(getMouseX());
        const float my = static_cast<float>(getMouseY());

        noFill();
        stroke(0, 255, 0, 255);
        circle(mx, my, 100.0f);

        next();
    }

    void destroy(AppContext& context, Next next) override
    {
        next();
    }
};

struct ExampleSketch : Sketch
{
    void plugins(Engine& engine) override
    {
        engine.addModule(std::make_unique<ExampleModule>());
    }

    void setup() override
    {
        info("ExampleSketch setup");
    }

    void draw() override
    {
    }
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<ExampleSketch>();
    }
} // namespace p5cpp
