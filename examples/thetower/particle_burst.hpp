#pragma once

#include <vector>

#include <p5cpp/p5cpp.hpp>

#include "death_animation.hpp"

struct Particle
{
    p5::float2 position;
    p5::float2 velocity;

    float size;
    float lifetime;
    float age;
};

struct ParticleBurst
{
    p5::color_t color;
    std::vector<Particle> particles;
};

ParticleBurst particle_burst_create(const ParticleDeathAnimation& animation, const p5::float2& position, float enemySize);
void particle_burst_update(ParticleBurst& burst, float deltaTime);
bool particle_burst_is_finished(const ParticleBurst& burst);
void particle_burst_show(const ParticleBurst& burst);
