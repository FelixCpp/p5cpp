#pragma once

#include <vector>

#include <p5cpp/p5cpp.hpp>

#include "game_events.hpp"
#include "tower.hpp"

// a shot on its way from an enemy to the tower, it is the only thing that damages the tower from a distance
struct EnemyShot
{
    EnemyId shooter;
    p5::float2 origin;
    p5::float2 target;
    p5::color_t color;

    float damage;
    float flightTime;
    float elapsed;
};

void enemy_shots_spawn(std::vector<EnemyShot>& shots, const Tower& tower, const GameEvents& events);
// landed shots strike the tower and are gone, the rest moves on
void enemy_shots_update(std::vector<EnemyShot>& shots, GameEvents& events, float deltaTime);
void enemy_shots_show(const std::vector<EnemyShot>& shots);
