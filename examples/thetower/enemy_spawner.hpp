#pragma once

#include <vector>

#include <p5cpp/p5cpp.hpp>

#include "enemy.hpp"

struct EnemySpawner
{
    p5::float2 center;
    float spawnDistance;

    float interval;
    float elapsed;

    EnemyId nextId;
};

void enemy_spawner_update(EnemySpawner& spawner, std::vector<Enemy>& enemies, float deltaTime);
