#include "melee_enemy.hpp"

#include "enemy.hpp"

using namespace p5;
using namespace p5::animation;

namespace
{
    constexpr color_t color = rgba(220, 180, 150);
}

Enemy melee_enemy_create(const EnemyId id, const p5::float2& position)
{
    return Enemy {
        .core = EnemyCore {
            .id = id,
            .health = EnemyHealth {
                .current = 30.0f,
                .pendingDamage = 0.0f,
            },
            .size = EnemySize {
                .size = valueTween(0.0f, 25.0f, 2.0f, curves::easeInOutCubic),
            },
            .movement = EnemyMovement {
                .position = position,
                .velocity = {},
                .acceleration = {},
                .maxSpeed = 80.0f,
                .maxForce = 150.0f,
            },
            .damageFeedback = enemy_damage_feedback_create(),
            .deathAnimation = ParticleDeathAnimation {
                .color = color,
            },
            .attackReach = 0.0f,
        },
        .behavior = MeleeBehavior {},
    };
}

void melee_enemy_update(EnemyCore& core, MeleeBehavior&, const Tower& tower, const float deltaTime)
{
    enemy_core_seek(core, tower.position);
    enemy_core_update(core, deltaTime);
}

void melee_enemy_show(const EnemyCore& core, const MeleeBehavior&)
{
    with([&core] {
        const float size = core.size.size.value();
        const auto [x, y] = core.movement.position;

        fill(enemy_damage_feedback_fill(core.damageFeedback, color));
        stroke(color);
        strokeWeight(3.0f);
        rect(x - size * 0.5f, y - size * 0.5f, size, size);
    });
}
