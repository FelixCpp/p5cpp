#pragma once

#include <vector>

#include <p5cpp/p5cpp.hpp>

#include "enemy.hpp"
#include "game_events.hpp"
#include "spawn_table.hpp"

struct WaveConfig
{
    p5::float2 center;
    float spawnDistance;

    int budgetBase;
    int budgetGrowthPerWave;

    // every wave lasts exactly this long, its enemies are spread evenly over it
    float waveDuration;
    int bossWaveInterval;

    std::vector<SpawnEntry> enemies;
    std::vector<SpawnEntry> bosses;
};

struct WaveSpawner
{
    int wave;
    float elapsed;

    std::vector<EnemyFactory> queue;
    size_t spawned;
    float spawnInterval;
    float spawnTimer;
};

struct WaveProgress
{
    float timeRemaining;
    float duration;
    int spawned;
    int total;
};

WaveSpawner wave_spawner_create(const WaveConfig& config);
WaveProgress wave_spawner_progress(const WaveSpawner& spawner, const WaveConfig& config);
void wave_spawner_update(WaveSpawner& spawner, const WaveConfig& config, std::vector<Enemy>& enemies, EnemyId& nextEnemyId, GameEvents& events, float deltaTime);
