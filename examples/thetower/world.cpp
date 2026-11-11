#include "world.hpp"

#include <algorithm>
#include <format>

using namespace p5;

namespace
{
    constexpr color_t goldColor = rgba(230, 200, 90);
}

World world_create()
{
    const float2 center = {300.0f, 300.0f};

    WaveConfig waveConfig = {
        .center = center,
        .spawnDistance = 280.0f,
        .budgetBase = 8,
        .budgetGrowthPerWave = 3,
        .spawnIntervalBase = 1.5f,
        .spawnIntervalDecayPerWave = 0.95f,
        .spawnIntervalMin = 0.3f,
        .breakDuration = 3.0f,
        .bossWaveInterval = 10,
        .enemies = {
            SpawnEntry {
                .archetype = &easyEnemy,
                .cost = 1,
                .weight = SpawnWeight {
                    .unlockWave = 1,
                    .baseWeight = 10.0f,
                    .growthPerWave = 0.0f,
                },
            },
            SpawnEntry {
                .archetype = &fastEnemy,
                .cost = 1,
                .weight = SpawnWeight {
                    .unlockWave = 2,
                    .baseWeight = 3.0f,
                    .growthPerWave = 0.5f,
                },
            },
            SpawnEntry {
                .archetype = &tankEnemy,
                .cost = 4,
                .weight = SpawnWeight {
                    .unlockWave = 4,
                    .baseWeight = 1.0f,
                    .growthPerWave = 0.4f,
                },
            },
        },
        .bosses = {},
    };
    WaveSpawner waves = wave_spawner_create(waveConfig);

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
            .goldPerWave = 5,
            .goldPerKill = 0,
            .gameSpeed = 1.0f,
        },
        .upgrades = UpgradeLevels {
            .levels = {},
        },
        .gold = 0,
        .waveConfig = std::move(waveConfig),
        .waves = std::move(waves),
        .nextEnemyId = 0,
        .enemies = {},
        .bullets = {},
        .deathEffects = {},
        .floatingTexts = {},
        .events = {},
    };
}

TowerStats world_tower_stats(const World& world)
{
    return tower_stats_compute(world.baseStats, world.upgrades);
}

void world_update(World& world, const float realDeltaTime)
{
    const TowerStats stats = world_tower_stats(world);
    const float deltaTime = realDeltaTime * stats.gameSpeed;

    game_events_clear(world.events);
    wave_spawner_update(world.waves, world.waveConfig, world.enemies, world.nextEnemyId, world.events, deltaTime);

    for (Enemy& enemy : world.enemies) {
        enemy_update(enemy, world.tower, deltaTime);
    }

    bullets_update(world.bullets, world.enemies, world.events, deltaTime);
    enemies_remove_dead(world.enemies);
    for (const DeathEvent& death : world.events.deaths) {
        const int reward = death.goldReward + stats.goldPerKill;
        world.gold += reward;
        world.floatingTexts.push_back(floating_text_create(std::format("+{}", reward), death.position, goldColor, std::min(16.0f + 2.0f * reward, 30.0f), 60.0f, 0.9f));
    }
    for (const WaveCompletedEvent& wave : world.events.wavesCompleted) {
        world.gold += stats.goldPerWave;
        world.floatingTexts.push_back(floating_text_create(std::format("Welle {} geschafft", wave.wave), world.tower.position + float2 {0.0f, -90.0f}, rgba(230), 28.0f, 20.0f, 2.0f));
        world.floatingTexts.push_back(floating_text_create(std::format("+{} G", stats.goldPerWave), world.tower.position + float2 {0.0f, -60.0f}, goldColor, 20.0f, 20.0f, 2.0f));
    }

    death_effects_spawn(world.deathEffects, world.events);
    death_effects_update(world.deathEffects, deltaTime);
    floating_texts_update(world.floatingTexts, deltaTime);
    tower_weapon_update(world.weapon, stats, world.tower, world.enemies, world.bullets, deltaTime);
}

void world_show(const World& world, const p5::Font& font)
{
    tower_show(world.tower, world_tower_stats(world).range);
    for (const Enemy& enemy : world.enemies) {
        enemy_show(enemy);
    }
    death_effects_show(world.deathEffects);
    for (const Bullet& bullet : world.bullets) {
        bullet_show(bullet);
    }
    floating_texts_show(world.floatingTexts, font);
}
