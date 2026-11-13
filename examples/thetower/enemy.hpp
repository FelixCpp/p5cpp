#pragma once

#include <variant>
#include <vector>

#include "enemy_core.hpp"
#include "game_events.hpp"
#include "melee_enemy.hpp"
#include "tower.hpp"

using EnemyBehavior = std::variant<MeleeBehavior>;

struct Enemy
{
    EnemyCore core;
    EnemyBehavior behavior;
};

Enemy* enemy_find(std::vector<Enemy>& enemies, EnemyId id);
void enemies_remove_dead(std::vector<Enemy>& enemies);
// kills everyone below the execute threshold, also those that fell under it without being hit
void enemies_execute(std::vector<Enemy>& enemies, float executeThreshold, GameEvents& events);
void enemy_update(Enemy& enemy, const Tower& tower, GameEvents& events, float deltaTime);
void enemy_show(const Enemy& enemy);
