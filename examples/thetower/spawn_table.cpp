#include "spawn_table.hpp"

using namespace p5;

float spawn_weight_at(const SpawnWeight& weight, const int wave)
{
    if (wave < weight.unlockWave) {
        return 0.0f;
    }
    return weight.baseWeight + weight.growthPerWave * static_cast<float>(wave - weight.unlockWave);
}

float spawn_table_chance(const std::vector<SpawnEntry>& entries, const SpawnEntry& entry, const int wave)
{
    float total = 0.0f;
    for (const SpawnEntry& other : entries) {
        total += spawn_weight_at(other.weight, wave);
    }

    return total > 0.0f ? spawn_weight_at(entry.weight, wave) / total : 0.0f;
}

const SpawnEntry* spawn_table_roll(const std::vector<SpawnEntry>& entries, const int wave, const int budget)
{
    float total = 0.0f;
    for (const SpawnEntry& entry : entries) {
        if (entry.cost <= budget) {
            total += spawn_weight_at(entry.weight, wave);
        }
    }

    if (total <= 0.0f) {
        return nullptr;
    }

    float roll = random(total);
    const SpawnEntry* last = nullptr;

    for (const SpawnEntry& entry : entries) {
        if (entry.cost > budget) {
            continue;
        }

        const float weight = spawn_weight_at(entry.weight, wave);
        if (weight <= 0.0f) {
            continue;
        }

        last = &entry;
        if (roll < weight) {
            return &entry;
        }
        roll -= weight;
    }

    // float rounding can leave the roll a hair above the last weight
    return last;
}
