#pragma once

#include <span>
#include <string>
#include <string_view>

#include "tower_stats.hpp"
#include "upgrade_levels.hpp"

enum class UpgradeCategory
{
    attack,
    utility,
};

struct UpgradeDefinition
{
    UpgradeId id;
    UpgradeCategory category;
    std::string_view name;

    int baseCost;
    float costGrowth;
    int maxLevel; // 0 = unlimited

    // formats the stat this upgrade improves, so the HUD can show "current -> next"
    std::string (*describe)(const TowerStats& stats);
};

std::span<const UpgradeDefinition> upgrade_definitions();
std::string_view upgrade_category_name(UpgradeCategory category);

int upgrade_cost(const UpgradeDefinition& definition, int level);
bool upgrade_is_maxed(const UpgradeDefinition& definition, int level);
bool upgrade_try_buy(const UpgradeDefinition& definition, UpgradeLevels& levels, int& gold);
