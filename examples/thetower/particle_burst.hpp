#pragma once

#include <vector>

#include <p5cpp/p5cpp.hpp>

#include "death_animation.hpp"

// a piece of the broken body, it tumbles outwards and keeps the enemy's color
struct Shard
{
    p5::float2 position;
    p5::float2 velocity;

    float size;
    p5::angle_t angle;
    p5::angle_t spin; // per second

    float lifetime;
    float age;
};

// a bright streak, much faster than a shard and gone much sooner
struct Spark
{
    p5::float2 position;
    p5::float2 velocity;

    float lifetime;
    float age;
};

struct ParticleBurst
{
    p5::color_t color;
    p5::float2 position;
    float size;

    // the flash and the shockwave are driven by the burst's own age, not by a particle
    float age;

    std::vector<Shard> shards;
    std::vector<Spark> sparks;
};

ParticleBurst particle_burst_create(const ParticleDeathAnimation& animation, const p5::float2& position, float enemySize);
void particle_burst_update(ParticleBurst& burst, float deltaTime);
bool particle_burst_is_finished(const ParticleBurst& burst);
void particle_burst_show(const ParticleBurst& burst);
