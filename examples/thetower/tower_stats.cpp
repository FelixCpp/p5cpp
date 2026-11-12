#include "tower_stats.hpp"

#include "upgrades.hpp"

TowerStats tower_stats_base()
{
    return TowerStats {
        .damage = 10.0f,
        .fireInterval = 0.2f,
        .bulletSpeed = 400.0f,
        .range = Meters {
            .value = 15.0f,
        },
        .critChance = 0.0f,
        .critMultiplier = 1.5f,
        .multishotChance = 0.0f,
        .multishotTargets = 2,
        .bounceChance = 0.0f,
        .bounceCount = 1,
        .bounceRange = Meters {
            .value = 4.0f,
        },
        .rapidFireChance = 0.0f,
        .rapidFireDuration = 1.0f,
        .rapidFireFactor = 4.0f,
        .maxHealth = 100.0f,
        .healthRegen = 0.0f,
        .percentDefense = 0.0f,
        .absoluteDefense = 0.0f,
        .thorns = 0.0f,
        .goldPerWave = 5,
        .goldPerKill = 0,
        .gameSpeed = 1.0f,
        .coinsPerWave = 1,
        .coinsPerKill = 0.05f,
    };
}

TowerStats tower_stats_compute(const TowerStats& base, const UpgradeLevels& levels)
{
    TowerStats stats = base;
    for (const UpgradeDefinition& definition : upgrade_definitions()) {
        definition.apply(stats, upgrade_level(levels, definition.id));
    }
    return stats;
}
