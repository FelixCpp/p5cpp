#pragma once

#include <vector>

#include <p5cpp/p5cpp.hpp>

#include "death_animation.hpp"
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
    int goldReward;
};

struct TowerHitEvent
{
    EnemyId attacker;
    p5::float2 position;
    float damage;
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
};

void game_events_clear(GameEvents& events);
