#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

#include "ui.hpp"
#include "upgrades.hpp"
#include "world.hpp"

struct Hud
{
    p5::float2 position;
    p5::float2 size;

    ui::Fonts fonts;
    UpgradeCategory activeCategory;
    ui::ScrollList upgradeScroll;

    // the shown gold rolls towards the real value and punches whenever gold comes in
    float displayedGold;
    int lastGold;
    p5::animation::ValueTweenTransition<float> goldPunch;
};

enum class HudAction
{
    none,
    openBestiary,
};

Hud hud_create(const ui::Fonts& fonts);
void hud_reset(Hud& hud);
HudAction hud_update(Hud& hud, World& world, float deltaTime);
void hud_show(const Hud& hud, const World& world);
