#include "melee_enemy.hpp"

#include <algorithm>
#include <cmath>

#include "enemy.hpp"
#include "palette.hpp"

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
        float weight;

        int goldReward;

        color_t color;
        EnemyShape shape;
        float strokeWeight;
    };

    constexpr MeleeEnemyStats easyStats = {
        .health = 30.0f,
        .damage = 5.0f,
        .attackInterval = 1.0f,
        .size = 25.0f,
        .maxSpeed = 80.0f,
        .maxForce = 150.0f,
        .weight = 1.0f,
        .goldReward = 1,
        .color = rgba(220, 180, 150),
        .shape = EnemyShape::square,
        .strokeWeight = 3.0f,
    };

    constexpr MeleeEnemyStats fastStats = {
        .health = 15.0f,
        .damage = 3.0f,
        .attackInterval = 0.6f,
        .size = 16.0f,
        .maxSpeed = 150.0f,
        .maxForce = 300.0f,
        .weight = 0.6f,
        .goldReward = 1,
        .color = rgba(230, 220, 110),
        .shape = EnemyShape::triangle,
        .strokeWeight = 2.0f,
    };

    constexpr MeleeEnemyStats tankStats = {
        .health = 120.0f,
        .damage = 20.0f,
        .attackInterval = 1.5f,
        .size = 40.0f,
        .maxSpeed = 40.0f,
        .maxForce = 80.0f,
        .weight = 3.0f,
        .goldReward = 5,
        .color = rgba(200, 90, 90),
        .shape = EnemyShape::hexagon,
        .strokeWeight = 5.0f,
    };

    Enemy melee_enemy_create(const MeleeEnemyStats& stats, const EnemyId id, const p5::float2& position)
    {
        return Enemy {
            .core = EnemyCore {
                .id = id,
                .health = EnemyHealth {
                    .current = stats.health,
                    .maxHealth = stats.health,
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
                    .push = {},
                    .slowFactor = 0.0f,
                    .slowLeft = 0.0f,
                },
                .damageFeedback = enemy_damage_feedback_create(),
                .deathAnimation = ParticleDeathAnimation {
                    .color = stats.color,
                },
                .attackReach = 0.0f,
                .weight = stats.weight,
                .goldReward = stats.goldReward,
            },
            .behavior = MeleeBehavior {
                .color = stats.color,
                .shape = stats.shape,
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
    .shape = easyStats.shape,
    .factory = easy_enemy_create,
    .health = easyStats.health,
    .damage = easyStats.damage,
};

const EnemyArchetype fastEnemy = {
    .name = "Fast",
    .color = fastStats.color,
    .shape = fastStats.shape,
    .factory = fast_enemy_create,
    .health = fastStats.health,
    .damage = fastStats.damage,
};

const EnemyArchetype tankEnemy = {
    .name = "Tank",
    .color = tankStats.color,
    .shape = tankStats.shape,
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
    enemy_core_update(core, deltaTime);

    // only the knockback still moves it, and it may shove the enemy out of contact
    const float2 pushed = movement.position - tower.position;
    if (lengthSquared(pushed) < contactDistance * contactDistance) {
        movement.position = tower.position + fixedLength(pushed, contactDistance);
    }

    behavior.attackCooldown -= deltaTime;
    while (behavior.attackCooldown <= 0.0f) {
        events.towerHits.push_back(TowerHitEvent {
            .attacker = core.id,
            .position = movement.position,
            .damage = behavior.damage,
        });
        behavior.attackCooldown += behavior.attackInterval;
    }
}

namespace
{
    angle_t body_rotation(const EnemyShape shape, const EnemyMovement& movement)
    {
        switch (shape) {
            case EnemyShape::triangle: return lengthSquared(movement.velocity) > 1.0f ? heading(movement.velocity) : degrees(0.0f);
            case EnemyShape::hexagon: return radians(static_cast<float>(getGlobalTime()) * 0.5f);
            case EnemyShape::square: break;
        }
        return degrees(0.0f);
    }

    // a ring that collapses onto the enemy while it grows to its full size
    void show_spawn_ring(const EnemyCore& core, const color_t color)
    {
        const float grown = core.size.size.progress();
        if (grown >= 1.0f) {
            return;
        }

        const auto [x, y] = core.movement.position;
        const float left = 1.0f - grown;

        noFill();
        stroke(withOpacity(color, 0.45f * left * left));
        strokeWeight(1.5f);
        circle(x, y, core.size.size.value() * 0.5f + 16.0f * left);
    }

    // only a hurt enemy carries a bar, an untouched crowd stays clean
    void show_health_bar(const EnemyCore& core, const float size)
    {
        const EnemyHealth& health = core.health;
        if (health.maxHealth <= 0.0f || health.current >= health.maxHealth) {
            return;
        }

        const float fraction = std::clamp(health.current / health.maxHealth, 0.0f, 1.0f);
        const float width = std::max(size * 0.9f, 14.0f);
        const float left = core.movement.position.x - width * 0.5f;
        const float top = core.movement.position.y + size * 0.5f + 7.0f;

        noStroke();
        fill(withOpacity(shadowColor, 0.55f));
        rect(left - 1.0f, top - 1.0f, width + 2.0f, 5.0f);
        fill(lerpColor(dangerColor, healthColor, fraction));
        rect(left, top, width * fraction, 3.0f);
    }
}

void melee_enemy_show(const EnemyCore& core, const MeleeBehavior& behavior)
{
    const float size = core.size.size.value();
    const auto [x, y] = core.movement.position;
    const float flash = core.damageFeedback.flash.value();

    // a slowed enemy is tinted towards frost, the stronger the slow the colder it looks
    const color_t color = lerpColor(behavior.color, slowColor, enemy_movement_slow_factor(core.movement));

    with([&] {
        noStroke();
        fill(withOpacity(shadowColor, 0.4f));
        ellipse(x, y + size * 0.44f, size * 0.46f, size * 0.18f);
    });

    with([&] {
        // a few layers instead of one disc, otherwise the halo shows its edge
        blendMode(BlendMode::additive);
        noStroke();
        fill(withOpacity(color, 0.035f));
        for (int i = 4; i >= 1; --i) {
            circle(x, y, size * 0.22f * static_cast<float>(i));
        }
    });

    with([&] {
        translate(x, y);
        rotate(body_rotation(behavior.shape, core.movement));
        // a hit punches the body a little bigger for as long as the flash lasts
        scale(1.0f + 0.22f * flash, 1.0f + 0.22f * flash);

        fill(lerpColor(rgba(20, 22, 28), color, 0.28f));
        stroke(color);
        strokeWeight(behavior.strokeWeight);
        enemy_shape_draw(behavior.shape, float2 {}, size);

        if (flash > 0.0f) {
            noStroke();
            fill(withOpacity(rgba(255), 0.7f * flash));
            enemy_shape_draw(behavior.shape, float2 {}, size);
        }
    });

    with([&] { show_spawn_ring(core, color); });
    with([&] { show_health_bar(core, size); });
}
