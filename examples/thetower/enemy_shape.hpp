#pragma once

#include <p5cpp/p5cpp.hpp>

// the body an enemy type is drawn as, it tells the types apart without a single texture
enum class EnemyShape
{
    square,   // plain, it just walks in
    triangle, // points the way it moves
    hexagon,  // heavy, turns slowly on its way
    diamond,  // small and quick
    pentagon, // carries a shield
    octagon,  // the heaviest body there is
    star,     // keeps its distance and shoots
    cross,    // falls apart into smaller pieces
};

// draws the outline with the current fill and stroke, on the playfield as well as in the interface
void enemy_shape_draw(EnemyShape shape, const p5::float2& center, float size);
