#pragma once

#include "meters.hpp"
#include "upgrade_levels.hpp"

struct TowerStats
{
    float damage;
    float fireInterval;
    float bulletSpeed;
    Meters range;
    float critChance;     // 0..1
    float critMultiplier; // damage factor of a critical hit

    float maxHealth;
    float healthRegen; // per second
    float percentDefense;  // 0..1, share of every enemy hit that is blocked
    float absoluteDefense; // subtracted from every enemy hit after the percent defense
    float thorns;          // 0..1, share of the unmitigated damage reflected to the attacker

    int goldPerWave;
    int goldPerKill; // added on top of every enemy's own reward
    float gameSpeed;

    int coinsPerWave;
    float coinsPerKill; // fractions add up over the round
};

TowerStats tower_stats_base();
TowerStats tower_stats_compute(const TowerStats& base, const UpgradeLevels& levels);
