#pragma once

#include "enemy_archetype.hpp"
#include "enemy_core.hpp"
#include "enemy_shape.hpp"
#include "game_events.hpp"
#include "tower.hpp"

struct Enemy;

// walks straight at the tower and hits it from its edge, the behaviour most types share
struct MeleeBehavior
{
    p5::color_t color;
    EnemyShape shape;
    float strokeWeight;

    float damage;
    float attackInterval;
    float attackCooldown;
};

Enemy grunt_enemy_create(EnemyId id, const p5::float2& position);
Enemy runner_enemy_create(EnemyId id, const p5::float2& position);
Enemy tank_enemy_create(EnemyId id, const p5::float2& position);
Enemy swarm_enemy_create(EnemyId id, const p5::float2& position);
Enemy splitter_enemy_create(EnemyId id, const p5::float2& position);
// what a splitter leaves behind, it never comes out of the spawn table itself
Enemy splinter_enemy_create(EnemyId id, const p5::float2& position);
Enemy guardian_enemy_create(EnemyId id, const p5::float2& position);
Enemy breaker_enemy_create(EnemyId id, const p5::float2& position);

extern const EnemyArchetype gruntEnemy;
extern const EnemyArchetype runnerEnemy;
extern const EnemyArchetype tankEnemy;
extern const EnemyArchetype swarmEnemy;
extern const EnemyArchetype splitterEnemy;
extern const EnemyArchetype guardianEnemy;
extern const EnemyArchetype breakerEnemy;

void melee_enemy_update(EnemyCore& core, MeleeBehavior& behavior, const Tower& tower, GameEvents& events, float deltaTime);
void melee_enemy_show(const EnemyCore& core, const MeleeBehavior& behavior);
