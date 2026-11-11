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

    float spawnIntervalBase;
    float spawnIntervalDecayPerWave;
    float spawnIntervalMin;

    float breakDuration;
    int bossWaveInterval;

    std::vector<SpawnEntry> enemies;
    std::vector<SpawnEntry> bosses;
};

enum class WavePhase
{
    spawning, // enemies of this wave are still being spawned
    clearing, // everything is spawned, waiting until the last enemy is dead
    pause,    // short break before the next wave starts
};

struct WaveSpawner
{
    int wave;
    WavePhase phase;
    int plannedCount;

    std::vector<EnemyFactory> queue;
    float spawnInterval;
    float elapsed;
};

struct WaveProgress
{
    WavePhase phase;
    int defeated;
    int total;
    float pauseRemaining;
};

WaveSpawner wave_spawner_create(const WaveConfig& config);
WaveProgress wave_spawner_progress(const WaveSpawner& spawner, const WaveConfig& config, size_t aliveEnemies);
void wave_spawner_update(WaveSpawner& spawner, const WaveConfig& config, std::vector<Enemy>& enemies, EnemyId& nextEnemyId, GameEvents& events, float deltaTime);
