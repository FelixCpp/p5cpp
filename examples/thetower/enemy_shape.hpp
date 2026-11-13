#pragma once

#include <p5cpp/p5cpp.hpp>

// the body an enemy type is drawn as, it tells the types apart without a single texture
enum class EnemyShape
{
    square,   // plain, it just walks in
    triangle, // points the way it moves
    hexagon,  // heavy, turns slowly on its way
};

// draws the outline with the current fill and stroke, on the playfield as well as in the interface
void enemy_shape_draw(EnemyShape shape, const p5::float2& center, float size);
