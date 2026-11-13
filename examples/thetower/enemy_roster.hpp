#pragma once

#include <span>

#include "spawn_table.hpp"

// every type the game can send, in the order it unlocks. The round spawns from it and the
// bestiary reads it, so both always talk about the same enemies.
std::span<const SpawnEntry> enemy_roster();
