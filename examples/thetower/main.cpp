#include <algorithm>

#include <p5cpp/p5cpp.hpp>

using namespace p5;

#include "hud.hpp"
#include "menu.hpp"
#include "profile.hpp"
#include "world.hpp"

namespace
{
    constexpr float hudWidth = 280.0f;
    constexpr const char* savePath = "thetower_save.txt";

    enum class Screen
    {
        menu,
        round,
    };
}

struct TheTower : Sketch
{
    Font font = loadFont("fonts/Lexend_Deca/static/LexendDeca-Bold.ttf").value();
    Profile profile = profile_load(savePath);

    Screen screen = Screen::menu;
    Menu menu = menu_create(font);

    World world = world_create(profile.startLevels);
    Hud hud = hud_create(font);
    bool roundSettled = false;

    void setup() override
    {
        setWindowSize(static_cast<uint32_t>(worldSize + hudWidth), static_cast<uint32_t>(worldSize));
        setWindowResizable(true);
    }

    void draw() override
    {
        background(rgba(31));

        switch (screen) {
            case Screen::menu: drawMenu(); break;
            case Screen::round: drawRound(); break;
        }
    }

    void drawMenu()
    {
        switch (menu_update(menu, profile)) {
            case MenuAction::none: break;
            case MenuAction::purchased: profile_save(profile, savePath); break;
            case MenuAction::startRound: {
                world = world_create(profile.startLevels);
                hud_reset(hud);
                roundSettled = false;
                screen = Screen::round;
                return;
            }
        }

        menu_show(menu, profile);
    }

    // the coins of a round are banked exactly once, as soon as the tower falls
    void settleRound()
    {
        profile.coins += world.coins;
        profile.bestWave = std::max(profile.bestWave, world.waves.wave);
        profile.lastWave = world.waves.wave;
        profile.lastCoins = world.coins;
        profile_save(profile, savePath);
        roundSettled = true;
    }

    void drawRound()
    {
        const float deltaTime = getDeltaTime();

        // the hud is docked to the right edge, the playfield takes whatever space is left
        const float width = getWidth();
        const float height = getHeight();
        const float playWidth = std::max(width - hudWidth, 1.0f);

        hud.position = float2 {playWidth, 0.0f};
        hud.size = float2 {std::min(hudWidth, width), height};

        if (world.gameOver) {
            if (!roundSettled) {
                settleRound();
            } else if (isMouseButtonPressed(MouseButton::left)) {
                screen = Screen::menu;
                menu.scroll = {};
                return;
            }
        }

        world_update(world, deltaTime);
        hud_update(hud, world, deltaTime);

        // fit the logical world square into the playfield and keep it centered
        const float scaleFactor = std::min(playWidth, height) / worldSize;
        const float offsetX = (playWidth - worldSize * scaleFactor) * 0.5f;
        const float offsetY = (height - worldSize * scaleFactor) * 0.5f;

        withClip(0.0f, 0.0f, playWidth, height, [&] {
            withMatrix([&] {
                translate(offsetX, offsetY);
                scale(scaleFactor, scaleFactor);
                world_show(world, font);
            });
        });

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
