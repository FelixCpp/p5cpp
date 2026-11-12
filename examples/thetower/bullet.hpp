#pragma once

#include <vector>

#include <p5cpp/p5cpp.hpp>

#include "enemy.hpp"
#include "game_events.hpp"
#include "hit.hpp"
#include "tower_stats.hpp"

enum class BulletKind
{
    shot, // fired by the tower
    bolt, // jumped on from an enemy that was hit
};

struct Bullet
{
    EnemyId target;
    Hit hit;

    p5::float2 origin;
    p5::float2 impact; // predicted meeting point with the target
    p5::float2 position;

    float flightTime;
    float elapsed;

    BulletKind kind;
    std::vector<EnemyId> chain; // enemies this bounce chain already hit, the target excluded; a chain never hits one twice
};

void bullets_update(std::vector<Bullet>& bullets, std::vector<Enemy>& enemies, const TowerStats& stats, GameEvents& events, float deltaTime);
void bullet_show(const Bullet& bullet);
