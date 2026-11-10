#pragma once

#include "enemy_size.hpp"
#include "enemy_movement.hpp"

struct EasyEnemy
{
    EnemySize size;
    EnemyMovement movement;
};

void easy_enemy_seek(EasyEnemy& enemy, const p5::float2& target);
void easy_enemy_update(EasyEnemy& enemy, float deltaTime);
void easy_enemy_show(const EasyEnemy& enemy);
