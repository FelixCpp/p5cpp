#pragma once

#include <span>
#include <string>
#include <string_view>

#include "tower_stats.hpp"
#include "upgrade_levels.hpp"

enum class UpgradeCategory
{
    attack,
    defense,
    utility,
};

struct UpgradeCost
{
    int base;
    float growth;
};

struct UpgradeDefinition
{
    UpgradeId id;
    UpgradeCategory category;
    std::string_view key; // stable name for the save file
    std::string_view name;

    UpgradeCost goldCost; // during a round, grows with the levels bought in that round
    UpgradeCost coinCost; // in the menu, grows with the start level
    int maxLevel;         // 0 = unlimited, counts start level and round levels together

    // formats the stat this upgrade improves, so the HUD can show "current -> next"
    std::string (*describe)(const TowerStats& stats);
};

std::span<const UpgradeDefinition> upgrade_definitions();
const UpgradeDefinition* upgrade_find(std::string_view key);
std::string_view upgrade_category_name(UpgradeCategory category);

int upgrade_cost(const UpgradeCost& cost, int level);
bool upgrade_is_maxed(const UpgradeDefinition& definition, int level);

// round purchase: raises the level for this round only
bool upgrade_buy_with_gold(const UpgradeDefinition& definition, UpgradeLevels& levels, const UpgradeLevels& startLevels, int& gold);
int upgrade_gold_price(const UpgradeDefinition& definition, const UpgradeLevels& levels, const UpgradeLevels& startLevels);

// menu purchase: raises the level every round starts with
bool upgrade_buy_with_coins(const UpgradeDefinition& definition, UpgradeLevels& startLevels, int& coins);
