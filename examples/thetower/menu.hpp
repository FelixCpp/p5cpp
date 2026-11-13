#pragma once

#include <p5cpp/p5cpp.hpp>

#include "profile.hpp"
#include "ui.hpp"
#include "upgrades.hpp"

struct Menu
{
    ui::Fonts fonts;
    UpgradeCategory activeCategory;
    ui::ScrollList scroll;
};

enum class MenuAction
{
    none,
    purchased,
    startRound,
};

Menu menu_create(const ui::Fonts& fonts);
MenuAction menu_update(Menu& menu, Profile& profile);
void menu_show(const Menu& menu, const Profile& profile);
