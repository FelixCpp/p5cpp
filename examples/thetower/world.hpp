#pragma once

#include <vector>

#include "bullet.hpp"
#include "death_effect.hpp"
#include "enemy.hpp"
#include "enemy_spawner.hpp"
#include "game_events.hpp"
#include "tower.hpp"
#include "tower_stats.hpp"
#include "tower_weapon.hpp"

struct World
{
    Tower tower;
    TowerWeapon weapon;
    TowerStats baseStats;
    UpgradeLevels upgrades;

    EnemySpawner spawner;
    std::vector<Enemy> enemies;
    std::vector<Bullet> bullets;
    std::vector<DeathEffect> deathEffects;

    GameEvents events;
};

World world_create();
TowerStats world_tower_stats(const World& world);
void world_update(World& world, float deltaTime);
void world_show(const World& world);
