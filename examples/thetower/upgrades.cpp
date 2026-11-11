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
            .key = "damage",
            .name = "Schaden",
            .goldCost = {.base = 10, .growth = 1.25f},
            .coinCost = {.base = 5, .growth = 1.3f},
            .maxLevel = 0,
            .describe = [](const TowerStats& stats) { return std::format("{:.0f}", stats.damage); },
        },
        UpgradeDefinition {
            .id = UpgradeId::attackSpeed,
            .category = UpgradeCategory::attack,
            .key = "attackSpeed",
            .name = "Angriffstempo",
            .goldCost = {.base = 15, .growth = 1.3f},
            .coinCost = {.base = 8, .growth = 1.35f},
            .maxLevel = 0,
            .describe = [](const TowerStats& stats) { return std::format("{:.1f}/s", 1.0f / stats.fireInterval); },
        },
        UpgradeDefinition {
            .id = UpgradeId::range,
            .category = UpgradeCategory::attack,
            .key = "range",
            .name = "Reichweite",
            .goldCost = {.base = 20, .growth = 1.35f},
            .coinCost = {.base = 10, .growth = 1.5f},
            .maxLevel = 8,
            .describe = [](const TowerStats& stats) { return std::format("{:.0f}", stats.range.value); },
        },
        UpgradeDefinition {
            .id = UpgradeId::maxHealth,
            .category = UpgradeCategory::defense,
            .key = "maxHealth",
            .name = "Max. Leben",
            .goldCost = {.base = 15, .growth = 1.3f},
            .coinCost = {.base = 5, .growth = 1.3f},
            .maxLevel = 0,
            .describe = [](const TowerStats& stats) { return std::format("{:.0f}", stats.maxHealth); },
        },
        UpgradeDefinition {
            .id = UpgradeId::healthRegen,
            .category = UpgradeCategory::defense,
            .key = "healthRegen",
            .name = "Regeneration",
            .goldCost = {.base = 20, .growth = 1.35f},
            .coinCost = {.base = 8, .growth = 1.35f},
            .maxLevel = 0,
            .describe = [](const TowerStats& stats) { return std::format("{:.1f}/s", stats.healthRegen); },
        },
        UpgradeDefinition {
            .id = UpgradeId::goldPerWave,
            .category = UpgradeCategory::utility,
            .key = "goldPerWave",
            .name = "Gold pro Welle",
            .goldCost = {.base = 25, .growth = 1.4f},
            .coinCost = {.base = 10, .growth = 1.4f},
            .maxLevel = 0,
            .describe = [](const TowerStats& stats) { return std::format("{}", stats.goldPerWave); },
        },
        UpgradeDefinition {
            .id = UpgradeId::goldPerKill,
            .category = UpgradeCategory::utility,
            .key = "goldPerKill",
            .name = "Gold pro Gegner",
            .goldCost = {.base = 40, .growth = 1.6f},
            .coinCost = {.base = 15, .growth = 1.5f},
            .maxLevel = 0,
            .describe = [](const TowerStats& stats) { return std::format("+{}", stats.goldPerKill); },
        },
        UpgradeDefinition {
            .id = UpgradeId::coinsPerWave,
            .category = UpgradeCategory::utility,
            .key = "coinsPerWave",
            .name = "Coins pro Welle",
            .goldCost = {.base = 60, .growth = 1.6f},
            .coinCost = {.base = 12, .growth = 1.5f},
            .maxLevel = 0,
            .describe = [](const TowerStats& stats) { return std::format("{}", stats.coinsPerWave); },
        },
        UpgradeDefinition {
            .id = UpgradeId::coinsPerKill,
            .category = UpgradeCategory::utility,
            .key = "coinsPerKill",
            .name = "Coins pro Gegner",
            .goldCost = {.base = 80, .growth = 1.7f},
            .coinCost = {.base = 15, .growth = 1.5f},
            .maxLevel = 0,
            .describe = [](const TowerStats& stats) { return std::format("{:.2f}", stats.coinsPerKill); },
        },
        UpgradeDefinition {
            .id = UpgradeId::gameSpeed,
            .category = UpgradeCategory::utility,
            .key = "gameSpeed",
            .name = "Spieltempo",
            .goldCost = {.base = 30, .growth = 1.5f},
            .coinCost = {.base = 20, .growth = 1.8f},
            .maxLevel = 4,
            .describe = [](const TowerStats& stats) { return std::format("{:.2f}x", stats.gameSpeed); },
        },
    };

    int& level_of(UpgradeLevels& levels, const UpgradeId id)
    {
        return levels.levels[static_cast<size_t>(id)];
    }
}

std::span<const UpgradeDefinition> upgrade_definitions()
{
    return definitions;
}

const UpgradeDefinition* upgrade_find(const std::string_view key)
{
    for (const UpgradeDefinition& definition : definitions) {
        if (definition.key == key) {
            return &definition;
        }
    }
    return nullptr;
}

std::string_view upgrade_category_name(const UpgradeCategory category)
{
    switch (category) {
        case UpgradeCategory::attack: return "Angriff";
        case UpgradeCategory::defense: return "Abwehr";
        case UpgradeCategory::utility: return "Utility";
    }
    return "";
}

int upgrade_cost(const UpgradeCost& cost, const int level)
{
    return static_cast<int>(std::round(cost.base * std::pow(cost.growth, static_cast<float>(level))));
}

bool upgrade_is_maxed(const UpgradeDefinition& definition, const int level)
{
    return definition.maxLevel > 0 && level >= definition.maxLevel;
}

int upgrade_gold_price(const UpgradeDefinition& definition, const UpgradeLevels& levels, const UpgradeLevels& startLevels)
{
    const int boughtThisRound = upgrade_level(levels, definition.id) - upgrade_level(startLevels, definition.id);
    return upgrade_cost(definition.goldCost, boughtThisRound);
}

bool upgrade_buy_with_gold(const UpgradeDefinition& definition, UpgradeLevels& levels, const UpgradeLevels& startLevels, int& gold)
{
    const int price = upgrade_gold_price(definition, levels, startLevels);
    int& level = level_of(levels, definition.id);

    if (upgrade_is_maxed(definition, level) || gold < price) {
        return false;
    }

    gold -= price;
    ++level;
    return true;
}

bool upgrade_buy_with_coins(const UpgradeDefinition& definition, UpgradeLevels& startLevels, int& coins)
{
    int& level = level_of(startLevels, definition.id);
    const int price = upgrade_cost(definition.coinCost, level);

    if (upgrade_is_maxed(definition, level) || coins < price) {
        return false;
    }

    coins -= price;
    ++level;
    return true;
}
