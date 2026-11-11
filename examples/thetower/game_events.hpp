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
    bool killed;
};

struct DeathEvent
{
    EnemyId enemy;
    p5::float2 position;
    float size;
    DeathAnimation animation;
};

struct GameEvents
{
    std::vector<DamageEvent> damage;
    std::vector<DeathEvent> deaths;
};

void game_events_clear(GameEvents& events);
