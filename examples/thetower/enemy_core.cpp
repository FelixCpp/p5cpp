#include "enemy_core.hpp"

using namespace p5;

void enemy_core_seek(EnemyCore& core, const p5::float2& target)
{
    EnemyMovement& movement = core.movement;

    const float2 desired = fixedLength(target - movement.position, enemy_movement_speed(movement));
    const float2 steering = limited(desired - movement.velocity, movement.maxForce);

    movement.acceleration += steering;
}

void enemy_core_update(EnemyCore& core, const float deltaTime)
{
    enemy_size_update(core.size, deltaTime);
    enemy_movement_update(core.movement, deltaTime);
    enemy_damage_feedback_update(core.damageFeedback, deltaTime);
}

void enemy_core_apply_hit(EnemyCore& core, const Hit& hit, GameEvents& events)
{
    if (enemy_health_is_dead(core.health)) {
        return;
    }

    if (hit.reserved) {
        enemy_health_apply(core.health, hit.damage);
    } else {
        enemy_health_apply_unreserved(core.health, hit.damage);
    }
    enemy_damage_feedback_hit(core.damageFeedback);

    if (hit.knockback > 0.0f) {
        enemy_movement_push(core.movement, hit.direction, hit.knockback / core.weight);
    }
    if (hit.slowFactor > 0.0f) {
        enemy_movement_slow(core.movement, hit.slowFactor, hit.slowDuration);
    }

    const bool killed = enemy_health_is_dead(core.health);
    events.damage.push_back(DamageEvent {
        .enemy = core.id,
        .position = core.movement.position,
        .amount = hit.damage,
        .isCrit = hit.isCrit,
        .killed = killed,
    });

    if (killed) {
        enemy_core_kill(core, events);
    }
}

void enemy_core_kill(EnemyCore& core, GameEvents& events)
{
    core.health.current = 0.0f;
    events.deaths.push_back(DeathEvent {
        .enemy = core.id,
        .position = core.movement.position,
        .size = core.size.size.value(),
        .animation = core.deathAnimation,
        .spawn = core.deathSpawn,
        .goldReward = core.goldReward,
    });
}

float enemy_core_time_to_reach(const EnemyCore& core, const Tower& tower)
{
    return enemy_movement_time_to_reach(core.movement, tower.position, tower.radius + core.attackReach);
}
