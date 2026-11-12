#include "particle_burst.hpp"

#include <algorithm>
#include <cmath>

#include <p5cpp_animation/p5cpp_animation.hpp>

using namespace p5;
using namespace p5::animation;

namespace
{
    // the particle count scales with the enemy's area, so twice the size bursts into four times the particles
    constexpr float areaPerParticle = 50.0f;
    constexpr int minParticles = 3;
    constexpr int maxParticles = 60;

    constexpr float drag = 4.0f;
} // namespace

ParticleBurst particle_burst_create(const ParticleDeathAnimation& animation, const p5::float2& position, const float enemySize)
{
    const int count = std::clamp(static_cast<int>(std::round(enemySize * enemySize / areaPerParticle)), minParticles, maxParticles);

    ParticleBurst burst = {
        .color = animation.color,
        .particles = {},
    };

    burst.particles.reserve(count);

    for (int i = 0; i < count; ++i) {
        const float speed = random(40.0f, 160.0f);
        const float2 spread = float2 {random(-0.5f, 0.5f), random(-0.5f, 0.5f)} * enemySize;

        burst.particles.push_back(Particle {
            .position = position + spread,
            .velocity = random2D(speed),
            .size = random(2.0f, 5.0f),
            .lifetime = random(0.4f, 0.8f),
            .age = 0.0f,
        });
    }

    return burst;
}

void particle_burst_update(ParticleBurst& burst, const float deltaTime)
{
    for (Particle& particle : burst.particles) {
        particle.age += deltaTime;
        particle.velocity = damp(particle.velocity, float2 {}, drag, deltaTime);
        particle.position += particle.velocity * deltaTime;
    }

    std::erase_if(burst.particles, [](const Particle& particle) {
        return particle.age >= particle.lifetime;
    });
}

bool particle_burst_is_finished(const ParticleBurst& burst)
{
    return burst.particles.empty();
}

void particle_burst_show(const ParticleBurst& burst)
{
    with([&burst] {
        for (const Particle& particle : burst.particles) {
            const float alpha = 1.0f - curves::easeInQuad(particle.age / particle.lifetime);

            stroke(withOpacity(burst.color, alpha));
            strokeWeight(particle.size);
            point(particle.position.x, particle.position.y);
        }
    });
}
