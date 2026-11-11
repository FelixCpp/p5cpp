#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

#include "upgrades.hpp"
#include "world.hpp"

enum class HudPage
{
    attack,
    utility,
    enemies,
};

struct Hud
{
    p5::float2 position;
    p5::float2 size;

    p5::Font font;
    HudPage activePage;

    // the shown gold rolls towards the real value and punches whenever gold comes in
    float displayedGold;
    int lastGold;
    p5::animation::ValueTweenTransition<float> goldPunch;
};

Hud hud_create(const p5::Font& font, const p5::float2& position, const p5::float2& size);
void hud_update(Hud& hud, World& world, float deltaTime);
void hud_show(const Hud& hud, const World& world);
