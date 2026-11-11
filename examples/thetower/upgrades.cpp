#include "upgrades.hpp"

#include <array>
#include <cmath>
#include <format>

namespace
{
    constexpr std::array definitions = {
        UpgradeDefinition {
            .id = UpgradeId::damage,
            .category = UpgradeCategory::attack,
            .name = "Schaden",
            .baseCost = 10,
            .costGrowth = 1.25f,
            .maxLevel = 0,
            .describe = [](const TowerStats& stats) { return std::format("{:.0f}", stats.damage); },
        },
        UpgradeDefinition {
            .id = UpgradeId::attackSpeed,
            .category = UpgradeCategory::attack,
            .name = "Angriffstempo",
            .baseCost = 15,
            .costGrowth = 1.3f,
            .maxLevel = 0,
            .describe = [](const TowerStats& stats) { return std::format("{:.1f}/s", 1.0f / stats.fireInterval); },
        },
        UpgradeDefinition {
            .id = UpgradeId::range,
            .category = UpgradeCategory::attack,
            .name = "Reichweite",
            .baseCost = 20,
            .costGrowth = 1.35f,
            .maxLevel = 8,
            .describe = [](const TowerStats& stats) { return std::format("{:.0f}", stats.range.value); },
        },
        UpgradeDefinition {
            .id = UpgradeId::goldPerWave,
            .category = UpgradeCategory::utility,
            .name = "Gold pro Welle",
            .baseCost = 25,
            .costGrowth = 1.4f,
            .maxLevel = 0,
            .describe = [](const TowerStats& stats) { return std::format("{}", stats.goldPerWave); },
        },
        UpgradeDefinition {
            .id = UpgradeId::goldPerKill,
            .category = UpgradeCategory::utility,
            .name = "Gold pro Gegner",
            .baseCost = 40,
            .costGrowth = 1.6f,
            .maxLevel = 0,
            .describe = [](const TowerStats& stats) { return std::format("+{}", stats.goldPerKill); },
        },
        UpgradeDefinition {
            .id = UpgradeId::gameSpeed,
            .category = UpgradeCategory::utility,
            .name = "Spieltempo",
            .baseCost = 30,
            .costGrowth = 1.5f,
            .maxLevel = 4,
            .describe = [](const TowerStats& stats) { return std::format("{:.2f}x", stats.gameSpeed); },
        },
    };
}

std::span<const UpgradeDefinition> upgrade_definitions()
{
    return definitions;
}

std::string_view upgrade_category_name(const UpgradeCategory category)
{
    switch (category) {
        case UpgradeCategory::attack: return "Angriff";
        case UpgradeCategory::utility: return "Utility";
    }
    return "";
}

int upgrade_cost(const UpgradeDefinition& definition, const int level)
{
    return static_cast<int>(std::round(definition.baseCost * std::pow(definition.costGrowth, static_cast<float>(level))));
}

bool upgrade_is_maxed(const UpgradeDefinition& definition, const int level)
{
    return definition.maxLevel > 0 && level >= definition.maxLevel;
}

bool upgrade_try_buy(const UpgradeDefinition& definition, UpgradeLevels& levels, int& gold)
{
    int& level = levels.levels[static_cast<size_t>(definition.id)];
    const int cost = upgrade_cost(definition, level);

    if (upgrade_is_maxed(definition, level) || gold < cost) {
        return false;
    }

    gold -= cost;
    ++level;
    return true;
}
