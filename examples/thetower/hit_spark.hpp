#pragma once

#include <vector>

#include <p5cpp/p5cpp.hpp>

#include "game_events.hpp"

// the short flare a landing bullet leaves on an enemy that survived it
struct HitSpark
{
    p5::float2 position;
    p5::color_t color;
    p5::angle_t rotation;

    float size;
    float lifetime;
    float age;
};

void hit_sparks_spawn(std::vector<HitSpark>& sparks, const GameEvents& events);
void hit_sparks_update(std::vector<HitSpark>& sparks, float deltaTime);
void hit_sparks_show(const std::vector<HitSpark>& sparks);
