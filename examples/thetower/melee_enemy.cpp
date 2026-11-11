#include "melee_enemy.hpp"

#include "enemy.hpp"

using namespace p5;
using namespace p5::animation;

namespace
{
    struct MeleeEnemyStats
    {
        float health;
        float damage;
        float attackInterval;
        float size;
        float maxSpeed;
        float maxForce;

        int goldReward;

        color_t color;
        float strokeWeight;
    };

    constexpr MeleeEnemyStats easyStats = {
        .health = 30.0f,
        .damage = 5.0f,
        .attackInterval = 1.0f,
        .size = 25.0f,
        .maxSpeed = 80.0f,
        .maxForce = 150.0f,
        .goldReward = 1,
        .color = rgba(220, 180, 150),
        .strokeWeight = 3.0f,
    };

    constexpr MeleeEnemyStats fastStats = {
        .health = 15.0f,
        .damage = 3.0f,
        .attackInterval = 0.6f,
        .size = 16.0f,
        .maxSpeed = 150.0f,
        .maxForce = 300.0f,
        .goldReward = 1,
        .color = rgba(230, 220, 110),
        .strokeWeight = 2.0f,
    };

    constexpr MeleeEnemyStats tankStats = {
        .health = 120.0f,
        .damage = 20.0f,
        .attackInterval = 1.5f,
        .size = 40.0f,
        .maxSpeed = 40.0f,
        .maxForce = 80.0f,
        .goldReward = 5,
        .color = rgba(200, 90, 90),
        .strokeWeight = 5.0f,
    };

    Enemy melee_enemy_create(const MeleeEnemyStats& stats, const EnemyId id, const p5::float2& position)
    {
        return Enemy {
            .core = EnemyCore {
                .id = id,
                .health = EnemyHealth {
                    .current = stats.health,
                    .pendingDamage = 0.0f,
                },
                .size = EnemySize {
                    .size = valueTween(0.0f, stats.size, 2.0f, curves::easeInOutCubic),
                },
                .movement = EnemyMovement {
                    .position = position,
                    .velocity = {},
                    .acceleration = {},
                    .maxSpeed = stats.maxSpeed,
                    .maxForce = stats.maxForce,
                },
                .damageFeedback = enemy_damage_feedback_create(),
                .deathAnimation = ParticleDeathAnimation {
                    .color = stats.color,
                },
                .attackReach = 0.0f,
                .goldReward = stats.goldReward,
            },
            .behavior = MeleeBehavior {
                .color = stats.color,
                .strokeWeight = stats.strokeWeight,
                .damage = stats.damage,
                .attackInterval = stats.attackInterval,
                .attackCooldown = 0.0f,
            },
        };
    }
}

Enemy easy_enemy_create(const EnemyId id, const p5::float2& position)
{
    return melee_enemy_create(easyStats, id, position);
}

Enemy fast_enemy_create(const EnemyId id, const p5::float2& position)
{
    return melee_enemy_create(fastStats, id, position);
}

Enemy tank_enemy_create(const EnemyId id, const p5::float2& position)
{
    return melee_enemy_create(tankStats, id, position);
}

const EnemyArchetype easyEnemy = {
    .name = "Easy",
    .color = easyStats.color,
    .factory = easy_enemy_create,
    .health = easyStats.health,
    .damage = easyStats.damage,
};

const EnemyArchetype fastEnemy = {
    .name = "Fast",
    .color = fastStats.color,
    .factory = fast_enemy_create,
    .health = fastStats.health,
    .damage = fastStats.damage,
};

const EnemyArchetype tankEnemy = {
    .name = "Tank",
    .color = tankStats.color,
    .factory = tank_enemy_create,
    .health = tankStats.health,
    .damage = tankStats.damage,
};

void melee_enemy_update(EnemyCore& core, MeleeBehavior& behavior, const Tower& tower, GameEvents& events, const float deltaTime)
{
    EnemyMovement& movement = core.movement;
    const float contactDistance = tower.radius + core.size.size.value() * 0.5f;
    const float2 offset = movement.position - tower.position;

    if (lengthSquared(offset) > contactDistance * contactDistance) {
        enemy_core_seek(core, tower.position);
        enemy_core_update(core, deltaTime);
        return;
    }

    // in contact: stand at the tower's edge and strike in a fixed rhythm
    movement.velocity = {};
    movement.acceleration = {};
    movement.position = tower.position + fixedLength(offset, contactDistance);
    enemy_core_update(core, deltaTime);

    behavior.attackCooldown -= deltaTime;
    while (behavior.attackCooldown <= 0.0f) {
        events.towerHits.push_back(TowerHitEvent {
            .position = movement.position,
            .damage = behavior.damage,
        });
        behavior.attackCooldown += behavior.attackInterval;
    }
}

void melee_enemy_show(const EnemyCore& core, const MeleeBehavior& behavior)
{
    with([&core, &behavior] {
        const float size = core.size.size.value();
        const auto [x, y] = core.movement.position;

        fill(enemy_damage_feedback_fill(core.damageFeedback, behavior.color));
        stroke(behavior.color);
        strokeWeight(behavior.strokeWeight);
        rect(x - size * 0.5f, y - size * 0.5f, size, size);
    });
}
