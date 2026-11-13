#pragma once

#include <p5cpp/p5cpp.hpp>

#include "ui.hpp"

// the full list of enemy types over the whole window: what every type is worth knowing, what it
// does differently, and either its share of the running wave or the wave it unlocks at

struct Bestiary
{
    ui::ScrollList scroll;
};

struct BestiaryLayout
{
    p5::rect2f panel;
    p5::rect2f list;
    p5::rect2f close;
    float contentHeight;
};

BestiaryLayout bestiary_layout(float width, float height);
// true once it should close again
bool bestiary_update(Bestiary& bestiary, const BestiaryLayout& layout);
// wave zero leaves out the shares, the menu has no running wave to report
void bestiary_show(const Bestiary& bestiary, const BestiaryLayout& layout, float width, float height, int wave);
