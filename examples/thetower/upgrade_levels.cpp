#include "upgrade_levels.hpp"

int upgrade_level(const UpgradeLevels& levels, const UpgradeId id)
{
    return levels.levels[static_cast<size_t>(id)];
}
