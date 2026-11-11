#include "tower_stats.hpp"

TowerStats tower_stats_compute(const TowerStats& base, const UpgradeLevels& levels)
{
    return TowerStats {
        .damage = base.damage + 5.0f * upgrade_level(levels, UpgradeId::damage),
        .fireInterval = base.fireInterval / (1.0f + 0.15f * upgrade_level(levels, UpgradeId::attackSpeed)),
        .bulletSpeed = base.bulletSpeed,
        .range = Meters {
            .value = base.range.value + 15.0f * upgrade_level(levels, UpgradeId::range),
        },
        .goldPerWave = base.goldPerWave + 3 * upgrade_level(levels, UpgradeId::goldPerWave),
        .goldPerKill = base.goldPerKill + upgrade_level(levels, UpgradeId::goldPerKill),
        .gameSpeed = base.gameSpeed + 0.25f * upgrade_level(levels, UpgradeId::gameSpeed),
    };
}
