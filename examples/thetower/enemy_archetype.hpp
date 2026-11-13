#pragma once

#include <string_view>

#include <p5cpp/p5cpp.hpp>

#include "death_spawn.hpp"
#include "enemy_shape.hpp"

// what the rest of the game needs to know about an enemy type without creating one
struct EnemyArchetype
{
    std::string_view name;
    std::string_view trait; // what sets the type apart, empty for the ones that only walk in and hit
    p5::color_t color;
    EnemyShape shape;
    EnemyFactory factory;

    float health;
    float shield; // zero for the types that carry none
    float damage;
    float speed; // pixels per second, before any slow
};
