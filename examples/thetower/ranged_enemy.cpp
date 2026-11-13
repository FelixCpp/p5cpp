#include "ranged_enemy.hpp"

#include "enemy.hpp"
#include "enemy_body.hpp"

using namespace p5;
using namespace p5::animation;

namespace
{
    constexpr float health = 55.0f;
    constexpr float shotDamage = 12.0f;
    constexpr float bodySize = 22.0f;
    constexpr float maxSpeed = 70.0f;
    constexpr float standoff = 150.0f;
    constexpr color_t bodyColor = rgba(190, 130, 240);

    // how close to its standoff the shooter accepts before it stops walking
    constexpr float standoffSlack = 12.0f;
}

Enemy shooter_enemy_create(const EnemyId id, const p5::float2& position)
{
    return Enemy {
        .core = EnemyCore {
            .id = id,
            .health = EnemyHealth {
                .current = health,
                .maxHealth = health,
                .shield = 0.0f,
                .maxShield = 0.0f,
                .pendingDamage = 0.0f,
            },
            .size = EnemySize {
                .size = valueTween(0.0f, bodySize, 2.0f, curves::easeInOutCubic),
            },
            .movement = EnemyMovement {
                .position = position,
                .velocity = {},
                .acceleration = {},
                .maxSpeed = maxSpeed,
                .maxForce = 130.0f,
                .push = {},
                .slowFactor = 0.0f,
                .slowLeft = 0.0f,
            },
            .damageFeedback = enemy_damage_feedback_create(),
            .deathAnimation = ParticleDeathAnimation {
                .color = bodyColor,
            },
            .deathSpawn = {},
            .attackReach = standoff,
            .weight = 1.2f,
            .goldReward = 4,
        },
        .behavior = RangedBehavior {
            .color = bodyColor,
            .shape = EnemyShape::star,
            .strokeWeight = 2.5f,
            .damage = shotDamage,
            .attackInterval = 2.0f,
            .attackCooldown = 1.0f,
            .standoff = standoff,
            .shotSpeed = 260.0f,
            .muzzleFlash = valueTweenJump(0.0f),
        },
    };
}

const EnemyArchetype shooterEnemy = {
    .name = "Schütze",
    .trait = "Hält Abstand und beschießt den Turm",
    .color = bodyColor,
    .shape = EnemyShape::star,
    .factory = shooter_enemy_create,
    .health = health,
    .shield = 0.0f,
    .damage = shotDamage,
    .speed = maxSpeed,
};

void ranged_enemy_update(EnemyCore& core, RangedBehavior& behavior, const Tower& tower, GameEvents& events, const float deltaTime)
{
    EnemyMovement& movement = core.movement;
    const float holdDistance = tower.radius + behavior.standoff;
    const float distance = length(movement.position - tower.position);

    behavior.muzzleFlash.advance(deltaTime);

    if (distance > holdDistance + standoffSlack) {
        enemy_core_seek(core, tower.position);
        enemy_core_update(core, deltaTime);
        return;
    }

    // in range: hold the ground and let only the knockback move it
    movement.velocity = {};
    movement.acceleration = {};
    enemy_core_update(core, deltaTime);

    behavior.attackCooldown -= deltaTime;
    while (behavior.attackCooldown <= 0.0f) {
        events.enemyShots.push_back(EnemyShotEvent {
            .shooter = core.id,
            .origin = movement.position,
            .color = behavior.color,
            .damage = behavior.damage,
            .speed = behavior.shotSpeed,
        });
        behavior.muzzleFlash = valueTween(1.0f, 0.0f, 0.25f, curves::easeOutQuad);
        behavior.attackCooldown += behavior.attackInterval;
    }
}

void ranged_enemy_show(const EnemyCore& core, const RangedBehavior& behavior)
{
    const float muzzle = behavior.muzzleFlash.value();

    enemy_body_show(core, EnemyBodyLook {
        .color = behavior.color,
        .shape = behavior.shape,
        .strokeWeight = behavior.strokeWeight,
        .rotation = radians(static_cast<float>(getGlobalTime()) * 0.4f),
        // the halo swells and the body kicks with every shot, so a shooter stands out of a crowd
        .extraGlow = muzzle,
        .extraScale = 0.12f * muzzle,
    });
}
