#include "enemy_core.hpp"

using namespace p5;

void enemy_core_seek(EnemyCore& core, const p5::float2& target)
{
    EnemyMovement& movement = core.movement;

    const float2 desired = fixedLength(target - movement.position, movement.maxSpeed);
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

    enemy_health_apply(core.health, hit.damage);
    enemy_damage_feedback_hit(core.damageFeedback);

    const bool killed = enemy_health_is_dead(core.health);
    events.damage.push_back(DamageEvent {
        .enemy = core.id,
        .position = core.movement.position,
        .amount = hit.damage,
        .isCrit = hit.isCrit,
        .killed = killed,
    });

    if (killed) {
        events.deaths.push_back(DeathEvent {
            .enemy = core.id,
            .position = core.movement.position,
            .size = core.size.size.value(),
            .animation = core.deathAnimation,
            .goldReward = core.goldReward,
        });
    }
}

float enemy_core_time_to_reach(const EnemyCore& core, const Tower& tower)
{
    return enemy_movement_time_to_reach(core.movement, tower.position, tower.radius + core.attackReach);
}
