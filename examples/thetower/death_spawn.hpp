#pragma once

#include <p5cpp/p5cpp.hpp>

#include "enemy_id.hpp"

struct Enemy;

using EnemyFactory = Enemy (*)(EnemyId id, const p5::float2& position);

// what an enemy leaves behind when it dies, nothing while the factory is null
struct DeathSpawn
{
    EnemyFactory factory;
    int count;
    float spread; // pixels the children are scattered around the death position
};
