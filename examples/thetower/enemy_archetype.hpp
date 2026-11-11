#pragma once

#include <string_view>

#include <p5cpp/p5cpp.hpp>

#include "enemy_id.hpp"

struct Enemy;

using EnemyFactory = Enemy (*)(EnemyId id, const p5::float2& position);

// what the rest of the game needs to know about an enemy type without creating one
struct EnemyArchetype
{
    std::string_view name;
    p5::color_t color;
    EnemyFactory factory;

    float health;
    float damage;
};
