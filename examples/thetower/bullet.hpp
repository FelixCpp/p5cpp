#pragma once

#include <vector>

#include <p5cpp/p5cpp.hpp>

#include "enemy.hpp"
#include "game_events.hpp"
#include "hit.hpp"

struct Bullet
{
    EnemyId target;
    Hit hit;

    p5::float2 origin;
    p5::float2 position;

    float flightTime;
    float elapsed;
};

void bullets_update(std::vector<Bullet>& bullets, std::vector<Enemy>& enemies, GameEvents& events, float deltaTime);
void bullet_show(const Bullet& bullet);
