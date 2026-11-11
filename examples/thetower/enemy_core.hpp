#pragma once

#include "death_animation.hpp"
#include "enemy_damage_feedback.hpp"
#include "enemy_health.hpp"
#include "enemy_id.hpp"
#include "enemy_movement.hpp"
#include "enemy_size.hpp"
#include "game_events.hpp"
#include "hit.hpp"
#include "tower.hpp"

struct EnemyCore
{
    EnemyId id;
    EnemyHealth health;
    EnemySize size;
    EnemyMovement movement;
    EnemyDamageFeedback damageFeedback;
    DeathAnimation deathAnimation;

    // distance to the tower's edge at which the enemy can attack it
    float attackReach;
};

void enemy_core_seek(EnemyCore& core, const p5::float2& target);
void enemy_core_update(EnemyCore& core, float deltaTime);
void enemy_core_apply_hit(EnemyCore& core, const Hit& hit, GameEvents& events);
float enemy_core_time_to_reach(const EnemyCore& core, const Tower& tower);
