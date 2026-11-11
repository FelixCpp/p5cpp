#include "death_effect.hpp"

#include <algorithm>

namespace
{
    template <typename... Ts>
    struct overloaded : Ts...
    {
        using Ts::operator()...;
    };
}

void death_effects_spawn(std::vector<DeathEffect>& effects, const GameEvents& events)
{
    for (const DeathEvent& event : events.deaths) {
        std::visit(
            overloaded {
                [&](const ParticleDeathAnimation& animation) {
                    effects.push_back(particle_burst_create(animation, event.position, event.size));
                },
            },
            event.animation
        );
    }
}

void death_effects_update(std::vector<DeathEffect>& effects, const float deltaTime)
{
    std::erase_if(effects, [deltaTime](DeathEffect& effect) {
        return std::visit(
            overloaded {
                [&](ParticleBurst& burst) {
                    particle_burst_update(burst, deltaTime);
                    return particle_burst_is_finished(burst);
                },
            },
            effect
        );
    });
}

void death_effects_show(const std::vector<DeathEffect>& effects)
{
    for (const DeathEffect& effect : effects) {
        std::visit(
            overloaded {
                [](const ParticleBurst& burst) { particle_burst_show(burst); },
            },
            effect
        );
    }
}
