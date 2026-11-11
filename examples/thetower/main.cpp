#include <p5cpp/p5cpp.hpp>

using namespace p5;

#include "hud.hpp"
#include "world.hpp"

struct TheTower : Sketch
{
    Font font = loadFont("fonts/Lexend_Deca/static/LexendDeca-Bold.ttf").value();
    World world = world_create();
    Hud hud = hud_create(font, float2 {600.0f, 0.0f}, float2 {260.0f, 600.0f});

    void setup() override
    {
        setWindowSize(860, 600);
        setWindowResizable(false);
    }

    void draw() override
    {
        const float deltaTime = getDeltaTime();
        world_update(world, deltaTime);
        hud_update(hud, world, deltaTime);

        background(rgba(31));
        world_show(world, font);
        hud_show(hud, world);
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
