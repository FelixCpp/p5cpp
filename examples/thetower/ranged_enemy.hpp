#pragma once

#include <p5cpp_animation/p5cpp_animation.hpp>

#include "enemy_archetype.hpp"
#include "enemy_core.hpp"
#include "enemy_shape.hpp"
#include "game_events.hpp"
#include "tower.hpp"

struct Enemy;

// holds at a distance from the tower and shoots at it instead of walking into its hull
struct RangedBehavior
{
    p5::color_t color;
    EnemyShape shape;
    float strokeWeight;

    float damage;
    float attackInterval;
    float attackCooldown;

    float standoff;  // distance to the tower's edge it stops at
    float shotSpeed; // pixels per second the shot travels

    p5::animation::ValueTweenTransition<float> muzzleFlash;
};

Enemy shooter_enemy_create(EnemyId id, const p5::float2& position);

extern const EnemyArchetype shooterEnemy;

void ranged_enemy_update(EnemyCore& core, RangedBehavior& behavior, const Tower& tower, GameEvents& events, float deltaTime);
void ranged_enemy_show(const EnemyCore& core, const RangedBehavior& behavior);
