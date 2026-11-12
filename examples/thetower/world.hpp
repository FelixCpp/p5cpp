#pragma once

#include <vector>

#include "bullet.hpp"
#include "death_effect.hpp"
#include "enemy.hpp"
#include "floating_text.hpp"
#include "game_events.hpp"
#include "tower.hpp"
#include "tower_stats.hpp"
#include "tower_weapon.hpp"
#include "upgrades.hpp"
#include "wave_spawner.hpp"

// the world is laid out in a fixed logical square, the sketch scales it to fit the window
inline constexpr float worldSize = 600.0f;
inline constexpr float worldMinSpeed = 0.5f;

struct World
{
    Tower tower;
    TowerWeapon weapon;
    TowerStats baseStats;
    UpgradeLevels startLevels;
    UpgradeLevels upgrades;
    int gold;

    int coins;
    float coinFraction;

    WaveConfig waveConfig;
    WaveSpawner waves;

    EnemyId nextEnemyId;
    std::vector<Enemy> enemies;
    std::vector<Bullet> bullets;
    std::vector<DeathEffect> deathEffects;
    std::vector<FloatingText> floatingTexts;

    // chosen by the player, capped by the max speed the upgrade unlocks
    float speed;

    GameEvents events;
    bool gameOver;
};

World world_create(const UpgradeLevels& startLevels);
TowerStats world_tower_stats(const World& world);
float world_speed(const World& world);
void world_change_speed(World& world, int steps);
bool world_buy_upgrade(World& world, const UpgradeDefinition& definition);
void world_update(World& world, float deltaTime);
void world_show(const World& world, const p5::Font& font);
