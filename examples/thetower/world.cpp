#include "world.hpp"

using namespace p5;

World world_create()
{
    const float2 center = {300.0f, 300.0f};

    return World {
        .tower = Tower {
            .position = center,
            .radius = 50.0f,
        },
        .weapon = TowerWeapon {
            .cooldown = 0.0f,
        },
        .baseStats = TowerStats {
            .damage = 10.0f,
            .fireInterval = 0.2f,
            .bulletSpeed = 400.0f,
            .range = Meters {
                .value = 150.0f,
            },
        },
        .upgrades = UpgradeLevels {
            .damage = 0,
            .fireRate = 0,
            .range = 0,
        },
        .spawner = EnemySpawner {
            .center = center,
            .spawnDistance = 280.0f,
            .interval = 1.5f,
            .elapsed = 0.0f,
            .nextId = 0,
        },
        .enemies = {},
        .bullets = {},
        .deathEffects = {},
        .events = {},
    };
}

TowerStats world_tower_stats(const World& world)
{
    return tower_stats_compute(world.baseStats, world.upgrades);
}

void world_update(World& world, const float deltaTime)
{
    const TowerStats stats = world_tower_stats(world);

    game_events_clear(world.events);
    enemy_spawner_update(world.spawner, world.enemies, deltaTime);

    for (Enemy& enemy : world.enemies) {
        enemy_update(enemy, world.tower, deltaTime);
    }

    bullets_update(world.bullets, world.enemies, world.events, deltaTime);
    enemies_remove_dead(world.enemies);
    death_effects_spawn(world.deathEffects, world.events);
    death_effects_update(world.deathEffects, deltaTime);
    tower_weapon_update(world.weapon, stats, world.tower, world.enemies, world.bullets, deltaTime);
}

void world_show(const World& world)
{
    tower_show(world.tower, world_tower_stats(world).range);
    for (const Enemy& enemy : world.enemies) {
        enemy_show(enemy);
    }
    death_effects_show(world.deathEffects);
    for (const Bullet& bullet : world.bullets) {
        bullet_show(bullet);
    }
}
