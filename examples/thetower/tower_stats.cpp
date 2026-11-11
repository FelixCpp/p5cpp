#include "tower_stats.hpp"

TowerStats tower_stats_compute(const TowerStats& base, const UpgradeLevels& levels)
{
    return TowerStats {
        .damage = base.damage + 5.0f * levels.damage,
        .fireInterval = base.fireInterval / (1.0f + 0.15f * levels.fireRate),
        .bulletSpeed = base.bulletSpeed,
        .range = Meters {
            .value = base.range.value + 15.0f * levels.range,
        },
    };
}
