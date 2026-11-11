#pragma once

#include <vector>

#include <p5cpp/p5cpp.hpp>

#include "enemy_archetype.hpp"

struct SpawnWeight
{
    int unlockWave;
    float baseWeight;
    float growthPerWave;
};

struct SpawnEntry
{
    const EnemyArchetype* archetype;
    int cost;
    SpawnWeight weight;
};

float spawn_weight_at(const SpawnWeight& weight, int wave);

// share of this entry among all entries unlocked at this wave, ignoring the budget
float spawn_table_chance(const std::vector<SpawnEntry>& entries, const SpawnEntry& entry, int wave);

// weighted roll over all entries unlocked at this wave that fit into the remaining budget
const SpawnEntry* spawn_table_roll(const std::vector<SpawnEntry>& entries, int wave, int budget);
