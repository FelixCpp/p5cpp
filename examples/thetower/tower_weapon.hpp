#pragma once

#include <vector>

#include "bullet.hpp"
#include "enemy.hpp"
#include "tower.hpp"
#include "tower_stats.hpp"

struct TowerWeapon
{
    float cooldown;
    float rapidFireLeft; // seconds of the running burst, 0 when none runs
};

void tower_weapon_update(TowerWeapon& weapon, const TowerStats& stats, const Tower& tower, std::vector<Enemy>& enemies, std::vector<Bullet>& bullets, float deltaTime);
void tower_weapon_show(const TowerWeapon& weapon, const TowerStats& stats, const Tower& tower);
