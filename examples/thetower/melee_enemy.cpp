#include "melee_enemy.hpp"


#include "enemy.hpp"
#include "enemy_body.hpp"

using namespace p5;
using namespace p5::animation;

namespace
{
    struct MeleeEnemyStats
    {
        float health;
        float shield;
        float damage;
        float attackInterval;
        float size;
        float maxSpeed;
        float maxForce;
        float weight;

        int goldReward;
        DeathSpawn deathSpawn;

        color_t color;
        EnemyShape shape;
        float strokeWeight;
    };

    constexpr MeleeEnemyStats gruntStats = {
        .health = 30.0f,
        .shield = 0.0f,
        .damage = 5.0f,
        .attackInterval = 1.0f,
        .size = 25.0f,
        .maxSpeed = 80.0f,
        .maxForce = 150.0f,
        .weight = 1.0f,
        .goldReward = 1,
        .deathSpawn = {},
        .color = rgba(220, 180, 150),
        .shape = EnemyShape::square,
        .strokeWeight = 3.0f,
    };

    constexpr MeleeEnemyStats runnerStats = {
        .health = 15.0f,
        .shield = 0.0f,
        .damage = 3.0f,
        .attackInterval = 0.6f,
        .size = 16.0f,
        .maxSpeed = 150.0f,
        .maxForce = 300.0f,
        .weight = 0.6f,
        .goldReward = 1,
        .deathSpawn = {},
        .color = rgba(230, 220, 110),
        .shape = EnemyShape::triangle,
        .strokeWeight = 2.0f,
    };

    constexpr MeleeEnemyStats tankStats = {
        .health = 120.0f,
        .shield = 0.0f,
        .damage = 20.0f,
        .attackInterval = 1.5f,
        .size = 40.0f,
        .maxSpeed = 40.0f,
        .maxForce = 80.0f,
        .weight = 3.0f,
        .goldReward = 5,
        .deathSpawn = {},
        .color = rgba(200, 90, 90),
        .shape = EnemyShape::hexagon,
        .strokeWeight = 5.0f,
    };

    constexpr MeleeEnemyStats swarmStats = {
        .health = 10.0f,
        .shield = 0.0f,
        .damage = 2.0f,
        .attackInterval = 0.5f,
        .size = 12.0f,
        .maxSpeed = 190.0f,
        .maxForce = 400.0f,
        .weight = 0.4f,
        .goldReward = 1,
        .deathSpawn = {},
        .color = rgba(150, 220, 170),
        .shape = EnemyShape::diamond,
        .strokeWeight = 2.0f,
    };

    // what a splitter falls apart into, it never comes out of the spawn table itself
    constexpr MeleeEnemyStats splinterStats = {
        .health = 18.0f,
        .shield = 0.0f,
        .damage = 4.0f,
        .attackInterval = 0.8f,
        .size = 14.0f,
        .maxSpeed = 140.0f,
        .maxForce = 280.0f,
        .weight = 0.5f,
        .goldReward = 1,
        .deathSpawn = {},
        .color = rgba(120, 200, 235),
        .shape = EnemyShape::cross,
        .strokeWeight = 2.0f,
    };

    constexpr MeleeEnemyStats splitterStats = {
        .health = 70.0f,
        .shield = 0.0f,
        .damage = 8.0f,
        .attackInterval = 1.1f,
        .size = 30.0f,
        .maxSpeed = 70.0f,
        .maxForce = 140.0f,
        .weight = 1.5f,
        .goldReward = 3,
        .deathSpawn = DeathSpawn {
            .factory = splinter_enemy_create,
            .count = 2,
            .spread = 18.0f,
        },
        .color = rgba(120, 200, 235),
        .shape = EnemyShape::cross,
        .strokeWeight = 3.0f,
    };

    constexpr MeleeEnemyStats guardianStats = {
        .health = 90.0f,
        .shield = 120.0f,
        .damage = 16.0f,
        .attackInterval = 1.3f,
        .size = 34.0f,
        .maxSpeed = 60.0f,
        .maxForce = 120.0f,
        .weight = 2.5f,
        .goldReward = 6,
        .deathSpawn = {},
        .color = rgba(110, 185, 205),
        .shape = EnemyShape::pentagon,
        .strokeWeight = 4.0f,
    };

    constexpr MeleeEnemyStats breakerStats = {
        .health = 300.0f,
        .shield = 0.0f,
        .damage = 40.0f,
        .attackInterval = 1.8f,
        .size = 50.0f,
        .maxSpeed = 30.0f,
        .maxForce = 60.0f,
        .weight = 5.0f,
        .goldReward = 12,
        .deathSpawn = {},
        .color = rgba(190, 70, 130),
        .shape = EnemyShape::octagon,
        .strokeWeight = 5.5f,
    };

    Enemy melee_enemy_create(const MeleeEnemyStats& stats, const EnemyId id, const p5::float2& position)
    {
        return Enemy {
            .core = EnemyCore {
                .id = id,
                .health = EnemyHealth {
                    .current = stats.health,
                    .maxHealth = stats.health,
                    .shield = stats.shield,
                    .maxShield = stats.shield,
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
                .deathSpawn = stats.deathSpawn,
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

    // the only shapes that are not drawn upright
    angle_t body_rotation(const EnemyShape shape, const EnemyMovement& movement)
    {
        switch (shape) {
            case EnemyShape::triangle: return lengthSquared(movement.velocity) > 1.0f ? heading(movement.velocity) : degrees(0.0f);
            case EnemyShape::diamond: return lengthSquared(movement.velocity) > 1.0f ? heading(movement.velocity) : degrees(0.0f);
            case EnemyShape::hexagon: return radians(static_cast<float>(getGlobalTime()) * 0.5f);
            case EnemyShape::octagon: return radians(static_cast<float>(getGlobalTime()) * 0.3f);
            case EnemyShape::cross: return radians(static_cast<float>(getGlobalTime()) * 0.8f);
            default: break;
        }
        return degrees(0.0f);
    }
}

Enemy grunt_enemy_create(const EnemyId id, const p5::float2& position)
{
    return melee_enemy_create(gruntStats, id, position);
}

Enemy runner_enemy_create(const EnemyId id, const p5::float2& position)
{
    return melee_enemy_create(runnerStats, id, position);
}

Enemy tank_enemy_create(const EnemyId id, const p5::float2& position)
{
    return melee_enemy_create(tankStats, id, position);
}

Enemy swarm_enemy_create(const EnemyId id, const p5::float2& position)
{
    return melee_enemy_create(swarmStats, id, position);
}

Enemy splinter_enemy_create(const EnemyId id, const p5::float2& position)
{
    return melee_enemy_create(splinterStats, id, position);
}

Enemy splitter_enemy_create(const EnemyId id, const p5::float2& position)
{
    return melee_enemy_create(splitterStats, id, position);
}

Enemy guardian_enemy_create(const EnemyId id, const p5::float2& position)
{
    return melee_enemy_create(guardianStats, id, position);
}

Enemy breaker_enemy_create(const EnemyId id, const p5::float2& position)
{
    return melee_enemy_create(breakerStats, id, position);
}

const EnemyArchetype gruntEnemy = {
    .name = "Grunzer",
    .trait = {},
    .color = gruntStats.color,
    .shape = gruntStats.shape,
    .factory = grunt_enemy_create,
    .health = gruntStats.health,
    .shield = gruntStats.shield,
    .damage = gruntStats.damage,
    .speed = gruntStats.maxSpeed,
};

const EnemyArchetype runnerEnemy = {
    .name = "Läufer",
    .trait = "Schnell, aber dünnhäutig",
    .color = runnerStats.color,
    .shape = runnerStats.shape,
    .factory = runner_enemy_create,
    .health = runnerStats.health,
    .shield = runnerStats.shield,
    .damage = runnerStats.damage,
    .speed = runnerStats.maxSpeed,
};

const EnemyArchetype tankEnemy = {
    .name = "Panzer",
    .trait = "Langsam und kaum zu verschieben",
    .color = tankStats.color,
    .shape = tankStats.shape,
    .factory = tank_enemy_create,
    .health = tankStats.health,
    .shield = tankStats.shield,
    .damage = tankStats.damage,
    .speed = tankStats.maxSpeed,
};

const EnemyArchetype swarmEnemy = {
    .name = "Schwarm",
    .trait = "Kommt in Massen und ist sofort da",
    .color = swarmStats.color,
    .shape = swarmStats.shape,
    .factory = swarm_enemy_create,
    .health = swarmStats.health,
    .shield = swarmStats.shield,
    .damage = swarmStats.damage,
    .speed = swarmStats.maxSpeed,
};

const EnemyArchetype splitterEnemy = {
    .name = "Teiler",
    .trait = "Zerfällt beim Tod in zwei Splitter",
    .color = splitterStats.color,
    .shape = splitterStats.shape,
    .factory = splitter_enemy_create,
    .health = splitterStats.health,
    .shield = splitterStats.shield,
    .damage = splitterStats.damage,
    .speed = splitterStats.maxSpeed,
};

const EnemyArchetype guardianEnemy = {
    .name = "Wächter",
    .trait = "Schild schluckt Schaden, bis es bricht",
    .color = guardianStats.color,
    .shape = guardianStats.shape,
    .factory = guardian_enemy_create,
    .health = guardianStats.health,
    .shield = guardianStats.shield,
    .damage = guardianStats.damage,
    .speed = guardianStats.maxSpeed,
};

const EnemyArchetype breakerEnemy = {
    .name = "Brecher",
    .trait = "Trifft härter als alles andere",
    .color = breakerStats.color,
    .shape = breakerStats.shape,
    .factory = breaker_enemy_create,
    .health = breakerStats.health,
    .shield = breakerStats.shield,
    .damage = breakerStats.damage,
    .speed = breakerStats.maxSpeed,
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

void melee_enemy_show(const EnemyCore& core, const MeleeBehavior& behavior)
{
    enemy_body_show(core, EnemyBodyLook {
        .color = behavior.color,
        .shape = behavior.shape,
        .strokeWeight = behavior.strokeWeight,
        .rotation = body_rotation(behavior.shape, core.movement),
        .extraGlow = 0.0f,
        .extraScale = 0.0f,
    });
}
