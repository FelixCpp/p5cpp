#pragma once

#include "enemy_core.hpp"
#include "tower.hpp"

struct Enemy;

struct MeleeBehavior
{
};

Enemy melee_enemy_create(EnemyId id, const p5::float2& position);
void melee_enemy_update(EnemyCore& core, MeleeBehavior& behavior, const Tower& tower, float deltaTime);
void melee_enemy_show(const EnemyCore& core, const MeleeBehavior& behavior);
