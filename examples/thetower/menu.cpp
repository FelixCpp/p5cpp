#include "menu.hpp"

#include <algorithm>
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

    struct Layout
    {
        rect2f column;
        rect2f startButton;
        std::vector<UpgradeTab> tabs;
        rect2f list;
    };

    // a centered column that never gets wider than maxColumnWidth
    Layout layout()
    {
        const float width = getWidth();
        const float height = getHeight();
        const float columnWidth = std::min(maxColumnWidth, std::max(width - padding * 2.0f, 1.0f));
        const float x = (width - columnWidth) * 0.5f;

        Layout result = {};
        result.column = rect2f {x, padding, columnWidth, height - padding * 2.0f};
        result.startButton = rect2f {x, padding + 70.0f, columnWidth, startButtonHeight};

        const float tabsY = result.startButton.top + startButtonHeight + 24.0f;
        result.tabs = upgrade_tabs_layout(rect2f {x, tabsY, columnWidth, tabHeight});

        const float listY = tabsY + tabHeight + gap * 2.0f;
        result.list = rect2f {x, listY, columnWidth, std::max(height - padding - listY, 0.0f)};
        return result;
    }

    UpgradeListLayout layout_upgrades(const Menu& menu, const rect2f& list)
    {
        return upgrade_list_layout(menu.activeCategory, list, menu.scroll.offset);
    }

    UpgradeLine describe_line(const UpgradeDefinition& definition, const Profile& profile)
    {
        const int price = upgrade_coin_price(definition, profile.startLevels);
        const UpgradeCurrency coins = {.symbol = "C", .color = coinColor, .balance = profile.coins};
        return upgrade_line_describe(definition, tower_stats_base(), profile.startLevels, "Start-Lv", price, coins);
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

    if (const std::optional<UpgradeCategory> category = upgrade_tabs_clicked(l.tabs)) {
        menu.activeCategory = *category;
        menu.scroll = {};
        return MenuAction::none;
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

    with([&] {
        textFont(menu.font);
        noStroke();

        textSize(26.0f);
        textAlign(TextAlignment::topLeft);
        fill(textColor);
        text("Dauerhafte Upgrades", l.column.left, l.column.top);

        textSize(22.0f);
        textAlign(TextAlignment::topRight);
        fill(coinColor);
        text(std::format("{} C", profile.coins), l.column.left + l.column.width, l.column.top + 2.0f);

        std::string summary = std::format("Beste Welle: {}", profile.bestWave);
        if (profile.lastWave > 0) {
            summary += std::format(" · Letzte Runde: Welle {}, +{} C", profile.lastWave, profile.lastCoins);
        }
        textSize(14.0f);
        textAlign(TextAlignment::topLeft);
        fill(secondaryTextColor);
        text(summary, l.column.left, l.column.top + 38.0f);

        button(l.startButton, "Runde starten", accentColor);

        upgrade_tabs_show(l.tabs, menu.activeCategory);
        upgrade_list_show(layout_upgrades(menu, l.list), l.list, menu.scroll, [&](const UpgradeDefinition& definition) {
            return describe_line(definition, profile);
        });
    });
}
