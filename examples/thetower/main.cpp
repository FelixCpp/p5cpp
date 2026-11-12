#include <algorithm>

#include <p5cpp/p5cpp.hpp>

using namespace p5;

#include "hud.hpp"
#include "menu.hpp"
#include "profile.hpp"
#include "round_controls.hpp"
#include "world.hpp"

namespace
{
    constexpr float hudWidth = 320.0f;
    constexpr float windowHeight = 720.0f; // taller than the world, so the hud has room for its upgrades
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
    bool paused = false;

    void setup() override
    {
        setWindowSize(static_cast<uint32_t>(worldSize + hudWidth), static_cast<uint32_t>(windowHeight));
        setWindowResizable(true);
        // escape pauses the round, it only quits from the menu
        setQuitOnEscape(false);
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
        if (isKeyPressed(Key::Escape)) {
            quit();
            return;
        }

        switch (menu_update(menu, profile)) {
            case MenuAction::none: break;
            case MenuAction::purchased: profile_save(profile, savePath); break;
            case MenuAction::startRound: {
                world = world_create(profile.startLevels);
                hud_reset(hud);
                roundSettled = false;
                paused = false;
                screen = Screen::round;
                return;
            }
        }

        menu_show(menu, profile);
    }

    // the coins of a round are banked exactly once, when the tower falls or the round is quit
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

        const RoundControlsLayout controls = round_controls_layout(float2 {12.0f, 12.0f});
        const PauseMenuLayout pauseMenu = pause_menu_layout(width, height);

        if (world.gameOver) {
            if (!roundSettled) {
                settleRound();
            } else if (isMouseButtonPressed(MouseButton::left) || isKeyPressed(Key::Escape)) {
                openMenu();
                return;
            }
        } else if (paused) {
            switch (pause_menu_update(pauseMenu)) {
                case RoundAction::resume: paused = false; break;
                case RoundAction::quitRound: {
                    settleRound();
                    openMenu();
                    return;
                }
                default: break;
            }
        } else {
            switch (round_controls_update(controls)) {
                case RoundAction::pause: paused = true; break;
                case RoundAction::slower: world_change_speed(world, -1); break;
                case RoundAction::faster: world_change_speed(world, 1); break;
                default: break;
            }
        }

        if (!paused) {
            world_update(world, deltaTime);
            hud_update(hud, world, deltaTime);
        }

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

        if (!world.gameOver) {
            round_controls_show(controls, world_speed(world), worldMinSpeed, world_tower_stats(world).gameSpeed);
        }
        if (paused) {
            pause_menu_show(pauseMenu, width, height);
        }
    }

    void openMenu()
    {
        paused = false;
        screen = Screen::menu;
        menu.scroll = {};
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
