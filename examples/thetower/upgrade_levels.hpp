#pragma once

#include <array>
#include <cstddef>

enum class UpgradeId
{
    damage,
    attackSpeed,
    range,
    critChance,
    critMultiplier,
    maxHealth,
    healthRegen,
    goldPerWave,
    goldPerKill,
    gameSpeed,
    coinsPerWave,
    coinsPerKill,
    count,
};

struct UpgradeLevels
{
    std::array<int, static_cast<size_t>(UpgradeId::count)> levels;
};

int upgrade_level(const UpgradeLevels& levels, UpgradeId id);
