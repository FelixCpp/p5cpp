#include <p5cpp/p5cpp.hpp>

using namespace p5;

#include "world.hpp"

struct TheTower : Sketch
{
    World world = world_create();

    void setup() override
    {
        setWindowSize(600, 600);
        setWindowResizable(false);
    }

    void draw() override
    {
        world_update(world, getDeltaTime());

        background(rgba(31));
        world_show(world);
    }
};

SketchSpec p5::createSpec()
{
    return {
        .plugins = {},
        .sketch = [] {
            return std::make_unique<TheTower>();
        }
    };
}
