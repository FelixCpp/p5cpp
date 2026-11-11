#include "wave_spawner.hpp"

#include <algorithm>
#include <cmath>
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

    float wave_spawn_interval(const WaveConfig& config, const int wave)
    {
        const float interval = config.spawnIntervalBase * std::pow(config.spawnIntervalDecayPerWave, static_cast<float>(wave - 1));
        return std::max(interval, config.spawnIntervalMin);
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

    void wave_start(WaveSpawner& spawner, const WaveConfig& config, const int wave)
    {
        spawner.wave = wave;
        spawner.phase = WavePhase::spawning;
        spawner.queue = wave_plan(config, wave);
        spawner.plannedCount = static_cast<int>(spawner.queue.size());
        spawner.spawnInterval = wave_spawn_interval(config, wave);
        spawner.elapsed = spawner.spawnInterval;
    }

    float2 spawn_position(const WaveConfig& config)
    {
        const float angle = random(TAU);
        return config.center + float2 {std::cos(angle), std::sin(angle)} * config.spawnDistance;
    }
}

WaveSpawner wave_spawner_create(const WaveConfig& config)
{
    WaveSpawner spawner = {};
    wave_start(spawner, config, 1);
    return spawner;
}

WaveProgress wave_spawner_progress(const WaveSpawner& spawner, const WaveConfig& config, const size_t aliveEnemies)
{
    const int outstanding = static_cast<int>(spawner.queue.size() + aliveEnemies);

    return WaveProgress {
        .phase = spawner.phase,
        .defeated = std::clamp(spawner.plannedCount - outstanding, 0, spawner.plannedCount),
        .total = spawner.plannedCount,
        .pauseRemaining = spawner.phase == WavePhase::pause ? std::max(config.breakDuration - spawner.elapsed, 0.0f) : 0.0f,
    };
}

void wave_spawner_update(WaveSpawner& spawner, const WaveConfig& config, std::vector<Enemy>& enemies, EnemyId& nextEnemyId, GameEvents& events, const float deltaTime)
{
    switch (spawner.phase) {
        case WavePhase::spawning: {
            spawner.elapsed += deltaTime;

            // the queue is spawned front to back, an index keeps the boss at the end
            size_t spawned = 0;
            while (spawned < spawner.queue.size() && spawner.elapsed >= spawner.spawnInterval) {
                spawner.elapsed -= spawner.spawnInterval;
                enemies.push_back(spawner.queue[spawned](nextEnemyId++, spawn_position(config)));
                ++spawned;
            }
            spawner.queue.erase(spawner.queue.begin(), spawner.queue.begin() + static_cast<std::ptrdiff_t>(spawned));

            if (spawner.queue.empty()) {
                spawner.phase = WavePhase::clearing;
            }
            break;
        }

        case WavePhase::clearing: {
            if (enemies.empty()) {
                events.wavesCompleted.push_back(WaveCompletedEvent {.wave = spawner.wave});
                spawner.phase = WavePhase::pause;
                spawner.elapsed = 0.0f;
            }
            break;
        }

        case WavePhase::pause: {
            spawner.elapsed += deltaTime;
            if (spawner.elapsed >= config.breakDuration) {
                wave_start(spawner, config, spawner.wave + 1);
            }
            break;
        }
    }
}
