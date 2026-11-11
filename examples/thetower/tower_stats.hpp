#pragma once

#include "meters.hpp"
#include "upgrade_levels.hpp"

struct TowerStats
{
    float damage;
    float fireInterval;
    float bulletSpeed;
    Meters range;

    float maxHealth;
    float healthRegen; // per second

    int goldPerWave;
    int goldPerKill; // added on top of every enemy's own reward
    float gameSpeed;

    int coinsPerWave;
    float coinsPerKill; // fractions add up over the round
};

TowerStats tower_stats_base();
TowerStats tower_stats_compute(const TowerStats& base, const UpgradeLevels& levels);
