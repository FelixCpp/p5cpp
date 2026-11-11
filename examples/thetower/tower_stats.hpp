#pragma once

#include "meters.hpp"
#include "upgrade_levels.hpp"

struct TowerStats
{
    float damage;
    float fireInterval;
    float bulletSpeed;
    Meters range;

    int goldPerWave;
    int goldPerKill; // added on top of every enemy's own reward
    float gameSpeed;
};

TowerStats tower_stats_compute(const TowerStats& base, const UpgradeLevels& levels);
