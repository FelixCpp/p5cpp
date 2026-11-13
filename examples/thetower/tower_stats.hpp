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
    float multishotChance; // 0..1, chance that a shot fires at several enemies at once
    int multishotTargets;  // enemies hit by such a shot, each gets one bullet
    float bounceChance;    // 0..1, chance that a bullet jumps on as a bolt after landing
    int bounceCount;       // jumps of such a bullet
    Meters bounceRange;    // max distance of a single jump
    float rapidFireChance;   // 0..1, chance that a shot starts a burst of rapid fire
    float rapidFireDuration; // seconds a burst lasts
    float rapidFireFactor;   // attack speed multiplier during a burst
    Meters knockback;        // distance a hit shoves an enemy of weight 1 away
    float slowFactor;        // 0..1, speed a hit enemy loses
    float slowDuration;      // seconds a slow lasts, refreshed by every hit
    float executeThreshold;  // 0..1 of its max health, below it an enemy dies right away

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
