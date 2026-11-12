#include "menu.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <vector>

#include "upgrade_list.hpp"

using namespace p5;
using namespace ui;

namespace
{
    constexpr float padding = 24.0f;
    constexpr float gap = 8.0f;
    constexpr float maxColumnWidth = 520.0f;
    constexpr float startButtonHeight = 52.0f;

    constexpr std::array categories = {UpgradeCategory::attack, UpgradeCategory::defense, UpgradeCategory::utility};

    struct Layout
    {
        Rect column;
        Rect startButton;
        std::array<Rect, categories.size()> tabs;
        Rect list;
    };

    // a centered column that never gets wider than maxColumnWidth
    Layout layout()
    {
        const float width = getWidth();
        const float height = getHeight();
        const float columnWidth = std::min(maxColumnWidth, std::max(width - padding * 2.0f, 1.0f));
        const float x = (width - columnWidth) * 0.5f;

        Layout result = {};
        result.column = Rect {x, padding, columnWidth, height - padding * 2.0f};
        result.startButton = Rect {x, padding + 70.0f, columnWidth, startButtonHeight};

        const float tabsY = result.startButton.y + startButtonHeight + 24.0f;
        const float tabWidth = (columnWidth - gap * static_cast<float>(categories.size() - 1)) / static_cast<float>(categories.size());
        for (size_t i = 0; i < categories.size(); ++i) {
            result.tabs[i] = Rect {x + static_cast<float>(i) * (tabWidth + gap), tabsY, tabWidth, tabHeight};
        }

        const float listY = tabsY + tabHeight + gap * 2.0f;
        result.list = Rect {x, listY, columnWidth, std::max(height - padding - listY, 0.0f)};
        return result;
    }

    UpgradeListLayout layout_upgrades(const Menu& menu, const Rect& list)
    {
        return upgrade_list_layout(menu.activeCategory, list, menu.scroll.offset);
    }

    UpgradeLine describe_line(const UpgradeDefinition& definition, const Profile& profile, const TowerStats& stats)
    {
        const int level = upgrade_level(profile.startLevels, definition.id);
        const bool maxed = upgrade_is_maxed(definition, level);
        const int price = upgrade_cost(definition.coinCost, level);

        UpgradeLevels nextLevels = profile.startLevels;
        ++nextLevels.levels[static_cast<size_t>(definition.id)];
        const TowerStats nextStats = tower_stats_compute(tower_stats_base(), nextLevels);

        return UpgradeLine {
            .name = definition.name,
            .levelLabel = std::format("Start-Lv {}", level),
            .value = definition.describe(stats),
            .nextValue = maxed ? std::string() : definition.describe(nextStats),
            .level = level,
            .maxLevel = definition.maxLevel,
            .price = maxed ? std::string("MAX") : std::format("{} C", price),
            .priceColor = coinColor,
            .affordable = !maxed && profile.coins >= price,
            .maxed = maxed,
        };
    }
}

Menu menu_create(const p5::Font& font)
{
    return Menu {
        .font = font,
        .activeCategory = UpgradeCategory::attack,
        .scroll = {},
    };
}

MenuAction menu_update(Menu& menu, Profile& profile)
{
    const Layout l = layout();
    scroll_list_update(menu.scroll, l.list, layout_upgrades(menu, l.list).contentHeight);

    if (clicked(l.startButton)) {
        return MenuAction::startRound;
    }

    for (size_t i = 0; i < categories.size(); ++i) {
        if (clicked(l.tabs[i])) {
            menu.activeCategory = categories[i];
            menu.scroll = {};
            return MenuAction::none;
        }
    }

    const UpgradeDefinition* definition = upgrade_list_clicked(layout_upgrades(menu, l.list), l.list);
    if (definition != nullptr && upgrade_buy_with_coins(*definition, profile.startLevels, profile.coins)) {
        return MenuAction::purchased;
    }

    return MenuAction::none;
}

void menu_show(const Menu& menu, const Profile& profile)
{
    const Layout l = layout();
    const TowerStats stats = tower_stats_compute(tower_stats_base(), profile.startLevels);

    with([&] {
        textFont(menu.font);
        noStroke();

        textSize(26.0f);
        textAlign(TextAlignment::topLeft);
        fill(textColor);
        text("Dauerhafte Upgrades", l.column.x, l.column.y);

        textSize(22.0f);
        textAlign(TextAlignment::topRight);
        fill(coinColor);
        text(std::format("{} C", profile.coins), l.column.x + l.column.w, l.column.y + 2.0f);

        std::string summary = std::format("Beste Welle: {}", profile.bestWave);
        if (profile.lastWave > 0) {
            summary += std::format(" · Letzte Runde: Welle {}, +{} C", profile.lastWave, profile.lastCoins);
        }
        textSize(14.0f);
        textAlign(TextAlignment::topLeft);
        fill(secondaryTextColor);
        text(summary, l.column.x, l.column.y + 38.0f);

        button(l.startButton, "Runde starten", accentColor);

        for (size_t i = 0; i < categories.size(); ++i) {
            tab(l.tabs[i], upgrade_category_name(categories[i]), categories[i] == menu.activeCategory);
        }

        upgrade_list_show(layout_upgrades(menu, l.list), l.list, menu.scroll, [&](const UpgradeDefinition& definition) {
            return describe_line(definition, profile, stats);
        });
    });
}
