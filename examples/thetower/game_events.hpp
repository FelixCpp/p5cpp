#pragma once

#include <vector>

#include <p5cpp/p5cpp.hpp>

#include "death_animation.hpp"
#include "death_spawn.hpp"
#include "enemy_id.hpp"

struct DamageEvent
{
    EnemyId enemy;
    p5::float2 position;
    float amount;
    bool isCrit;
    bool killed;
};

struct DeathEvent
{
    EnemyId enemy;
    p5::float2 position;
    float size;
    DeathAnimation animation;
    DeathSpawn spawn;
    int goldReward;
};

struct TowerHitEvent
{
    EnemyId attacker;
    p5::float2 position;
    float damage;
};

// a shot an enemy sent on its way towards the tower
struct EnemyShotEvent
{
    EnemyId shooter;
    p5::float2 origin;
    p5::color_t color;
    float damage;
    float speed;
};

struct WaveCompletedEvent
{
    int wave;
};

struct GameEvents
{
    std::vector<DamageEvent> damage;
    std::vector<DeathEvent> deaths;
    std::vector<WaveCompletedEvent> wavesCompleted;
    std::vector<TowerHitEvent> towerHits;
    std::vector<EnemyShotEvent> enemyShots;
};

void game_events_clear(GameEvents& events);
