#include "wave_spawner.hpp"

#include <algorithm>
#include <limits>

using namespace p5;

namespace
{
    bool is_boss_wave(const WaveConfig& config, const int wave)
    {
        return config.bossWaveInterval > 0 && wave % config.bossWaveInterval == 0;
    }

    int wave_budget(const WaveConfig& config, const int wave)
    {
        return config.budgetBase + config.budgetGrowthPerWave * (wave - 1);
    }

    std::vector<EnemyFactory> wave_plan(const WaveConfig& config, const int wave)
    {
        std::vector<EnemyFactory> queue;
        int budget = wave_budget(config, wave);

        // the boss is guaranteed regardless of its cost and arrives last, after the regular enemies
        const SpawnEntry* boss = is_boss_wave(config, wave) ? spawn_table_roll(config.bosses, wave, std::numeric_limits<int>::max()) : nullptr;
        if (boss != nullptr) {
            budget = std::max(budget - boss->cost, 0);
        }

        while (const SpawnEntry* entry = spawn_table_roll(config.enemies, wave, budget)) {
            queue.push_back(entry->archetype->factory);
            budget -= entry->cost;
        }

        if (boss != nullptr) {
            queue.push_back(boss->archetype->factory);
        }

        return queue;
    }

    void wave_start(WaveSpawner& spawner, const WaveConfig& config, const int wave, const float elapsed)
    {
        spawner.wave = wave;
        spawner.elapsed = elapsed;
        spawner.queue = wave_plan(config, wave);
        spawner.spawned = 0;
        spawner.spawnInterval = spawner.queue.empty() ? config.waveDuration : config.waveDuration / static_cast<float>(spawner.queue.size());

        // the first enemy of a wave spawns right away
        spawner.spawnTimer = spawner.spawnInterval + elapsed;
    }

    float2 spawn_position(const WaveConfig& config)
    {
        return config.center + random2D(config.spawnDistance);
    }
}

WaveSpawner wave_spawner_create(const WaveConfig& config)
{
    WaveSpawner spawner = {};
    wave_start(spawner, config, 1, 0.0f);
    return spawner;
}

WaveProgress wave_spawner_progress(const WaveSpawner& spawner, const WaveConfig& config)
{
    return WaveProgress {
        .timeRemaining = std::max(config.waveDuration - spawner.elapsed, 0.0f),
        .duration = config.waveDuration,
        .spawned = static_cast<int>(spawner.spawned),
        .total = static_cast<int>(spawner.queue.size()),
    };
}

void wave_spawner_update(WaveSpawner& spawner, const WaveConfig& config, std::vector<Enemy>& enemies, EnemyId& nextEnemyId, GameEvents& events, const float deltaTime)
{
    spawner.elapsed += deltaTime;
    spawner.spawnTimer += deltaTime;

    while (spawner.spawned < spawner.queue.size() && spawner.spawnTimer >= spawner.spawnInterval) {
        spawner.spawnTimer -= spawner.spawnInterval;
        enemies.push_back(spawner.queue[spawner.spawned++](nextEnemyId++, spawn_position(config)));
    }

    // the timer decides when a wave ends, enemies still alive simply overlap with the next one
    while (spawner.elapsed >= config.waveDuration) {
        while (spawner.spawned < spawner.queue.size()) {
            enemies.push_back(spawner.queue[spawner.spawned++](nextEnemyId++, spawn_position(config)));
        }

        events.wavesCompleted.push_back(WaveCompletedEvent {.wave = spawner.wave});
        wave_start(spawner, config, spawner.wave + 1, spawner.elapsed - config.waveDuration);
    }
}
