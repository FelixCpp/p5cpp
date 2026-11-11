#pragma once

#include "meters.hpp"

struct TowerStats
{
    float damage;
    float fireInterval;
    float bulletSpeed;
    Meters range;
};

struct UpgradeLevels
{
    int damage;
    int fireRate;
    int range;
};

TowerStats tower_stats_compute(const TowerStats& base, const UpgradeLevels& levels);
