#include "enemy_roster.hpp"

#include <array>

#include "melee_enemy.hpp"
#include "ranged_enemy.hpp"

namespace
{
    // cost is what the type takes out of a wave's budget, the weight how often it is rolled
    constexpr std::array roster = {
        SpawnEntry {
            .archetype = &gruntEnemy,
            .cost = 1,
            .weight = SpawnWeight {.unlockWave = 1, .baseWeight = 10.0f, .growthPerWave = 0.0f},
        },
        SpawnEntry {
            .archetype = &runnerEnemy,
            .cost = 1,
            .weight = SpawnWeight {.unlockWave = 2, .baseWeight = 3.0f, .growthPerWave = 0.5f},
        },
        SpawnEntry {
            .archetype = &tankEnemy,
            .cost = 4,
            .weight = SpawnWeight {.unlockWave = 4, .baseWeight = 1.0f, .growthPerWave = 0.4f},
        },
        SpawnEntry {
            .archetype = &swarmEnemy,
            .cost = 1,
            .weight = SpawnWeight {.unlockWave = 6, .baseWeight = 2.0f, .growthPerWave = 0.8f},
        },
        SpawnEntry {
            .archetype = &splitterEnemy,
            .cost = 3,
            .weight = SpawnWeight {.unlockWave = 8, .baseWeight = 1.0f, .growthPerWave = 0.35f},
        },
        SpawnEntry {
            .archetype = &guardianEnemy,
            .cost = 5,
            .weight = SpawnWeight {.unlockWave = 11, .baseWeight = 1.0f, .growthPerWave = 0.3f},
        },
        SpawnEntry {
            .archetype = &shooterEnemy,
            .cost = 4,
            .weight = SpawnWeight {.unlockWave = 14, .baseWeight = 1.0f, .growthPerWave = 0.3f},
        },
        SpawnEntry {
            .archetype = &breakerEnemy,
            .cost = 8,
            .weight = SpawnWeight {.unlockWave = 18, .baseWeight = 0.8f, .growthPerWave = 0.25f},
        },
    };
}

std::span<const SpawnEntry> enemy_roster()
{
    return roster;
}
