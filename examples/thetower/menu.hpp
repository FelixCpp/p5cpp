#pragma once

#include <p5cpp/p5cpp.hpp>

#include "profile.hpp"
#include "ui.hpp"
#include "upgrades.hpp"

struct Menu
{
    p5::Font font;
    UpgradeCategory activeCategory;
    ui::ScrollList scroll;
};

enum class MenuAction
{
    none,
    purchased,
    startRound,
};

Menu menu_create(const p5::Font& font);
MenuAction menu_update(Menu& menu, Profile& profile);
void menu_show(const Menu& menu, const Profile& profile);
