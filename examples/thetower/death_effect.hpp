#pragma once

#include <variant>
#include <vector>

#include "game_events.hpp"
#include "particle_burst.hpp"

using DeathEffect = std::variant<ParticleBurst>;

void death_effects_spawn(std::vector<DeathEffect>& effects, const GameEvents& events);
void death_effects_update(std::vector<DeathEffect>& effects, float deltaTime);
void death_effects_show(const std::vector<DeathEffect>& effects);
