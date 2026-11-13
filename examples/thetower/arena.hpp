#pragma once

#include <p5cpp/p5cpp.hpp>

// floor, grid and a glow around center, the look the whole game is drawn on; the menu puts its
// column on the same ground so both screens belong to one game
void arena_backdrop_show(const p5::rect2f& view, const p5::float2& center, float glowRadius);

// the floor of the round, drawn under everything: it fills whatever the window shows of the world,
// so no edge of the logical square appears, and carries the glow around the tower as well as the
// dashed ring the enemies walk in from
void arena_show(const p5::rect2f& view, const p5::float2& center, float spawnDistance);
