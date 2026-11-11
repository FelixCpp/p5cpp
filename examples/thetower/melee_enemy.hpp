#pragma once

#include "enemy_archetype.hpp"
#include "enemy_core.hpp"
#include "game_events.hpp"
#include "tower.hpp"

struct Enemy;

struct MeleeBehavior
{
    p5::color_t color;
    float strokeWeight;

    float damage;
    float attackInterval;
    float attackCooldown;
};

Enemy easy_enemy_create(EnemyId id, const p5::float2& position);
Enemy fast_enemy_create(EnemyId id, const p5::float2& position);
Enemy tank_enemy_create(EnemyId id, const p5::float2& position);

extern const EnemyArchetype easyEnemy;
extern const EnemyArchetype fastEnemy;
extern const EnemyArchetype tankEnemy;

void melee_enemy_update(EnemyCore& core, MeleeBehavior& behavior, const Tower& tower, GameEvents& events, float deltaTime);
void melee_enemy_show(const EnemyCore& core, const MeleeBehavior& behavior);
