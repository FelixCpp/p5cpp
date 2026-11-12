#include "tower_stats.hpp"

TowerStats tower_stats_base()
{
    return TowerStats {
        .damage = 10.0f,
        .fireInterval = 0.2f,
        .bulletSpeed = 400.0f,
        .range = Meters {
            .value = 150.0f,
        },
        .critChance = 0.0f,
        .critMultiplier = 1.5f,
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
    return TowerStats {
        .damage = base.damage + 5.0f * upgrade_level(levels, UpgradeId::damage),
        .fireInterval = base.fireInterval / (1.0f + 0.15f * upgrade_level(levels, UpgradeId::attackSpeed)),
        .bulletSpeed = base.bulletSpeed,
        .range = Meters {
            .value = base.range.value + 15.0f * upgrade_level(levels, UpgradeId::range),
        },
        .critChance = base.critChance + 0.05f * upgrade_level(levels, UpgradeId::critChance),
        .critMultiplier = base.critMultiplier + 0.35f * upgrade_level(levels, UpgradeId::critMultiplier),
        .maxHealth = base.maxHealth + 20.0f * upgrade_level(levels, UpgradeId::maxHealth),
        .healthRegen = base.healthRegen + 0.5f * upgrade_level(levels, UpgradeId::healthRegen),
        .percentDefense = base.percentDefense + 0.05f * upgrade_level(levels, UpgradeId::percentDefense),
        .absoluteDefense = base.absoluteDefense + 0.5f * upgrade_level(levels, UpgradeId::absoluteDefense),
        .thorns = base.thorns + 0.05f * upgrade_level(levels, UpgradeId::thorns),
        .goldPerWave = base.goldPerWave + 3 * upgrade_level(levels, UpgradeId::goldPerWave),
        .goldPerKill = base.goldPerKill + upgrade_level(levels, UpgradeId::goldPerKill),
        .gameSpeed = base.gameSpeed + 0.25f * upgrade_level(levels, UpgradeId::gameSpeed),
        .coinsPerWave = base.coinsPerWave + upgrade_level(levels, UpgradeId::coinsPerWave),
        .coinsPerKill = base.coinsPerKill + 0.05f * upgrade_level(levels, UpgradeId::coinsPerKill),
    };
}
