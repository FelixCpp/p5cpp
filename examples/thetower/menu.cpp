#include "menu.hpp"

#include <algorithm>
#include <format>
#include <vector>

#include "arena.hpp"
#include "upgrade_list.hpp"

using namespace p5;
using namespace ui;

namespace
{
    constexpr float padding = 24.0f;
    constexpr float gap = 8.0f;
    constexpr float maxColumnWidth = 520.0f;

    constexpr float markRadius = 21.0f;
    constexpr float titleHeight = 54.0f;
    constexpr float statHeight = 58.0f;
    constexpr float startButtonHeight = 54.0f;

    struct Layout
    {
        rect2f column;
        rect2f title;
        rect2f stats;
        rect2f startButton;
        rect2f bestiaryButton;
        std::vector<UpgradeTab> tabs;
        rect2f list;
    };

    // a centered column that never gets wider than maxColumnWidth
    Layout layout()
    {
        const float width = getWidth();
        const float height = getHeight();
        const float columnWidth = std::min(maxColumnWidth, std::max(width - padding * 2.0f, 1.0f));

        rect2f area {(width - columnWidth) * 0.5f, padding, columnWidth, height - padding * 2.0f};

        Layout result = {};
        result.column = area;
        result.title = cutTop(area, titleHeight);
        cutTop(area, 20.0f);
        result.stats = cutTop(area, statHeight);
        cutTop(area, 20.0f);
        rect2f actions = cutTop(area, startButtonHeight);
        result.bestiaryButton = cutRight(actions, 130.0f);
        cutRight(actions, gap);
        result.startButton = actions;
        cutTop(area, 24.0f);
        result.tabs = upgrade_tabs_layout(cutTop(area, tabHeight));
        cutTop(area, gap * 2.0f);
        result.list = area;
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

    // the hexagon of the tower stands in for a logo, down to the ring turning inside it
    void show_mark(const float2& position, const float radius)
    {
        const auto hexagon = [&](const float scale, const angle_t rotation) {
            beginShape(ShapeMode::path);
            for (size_t i = 0; i < 6; ++i) {
                const float2 corner = position + fromAngle(rotation + degrees(60.0f * i), radius * scale);
                vertex(corner.x, corner.y);
            }
            endShape(true);
        };

        glow(rect2f {position.x - radius, position.y - radius, radius * 2.0f, radius * 2.0f}, accentColor, 1.2f, radius, 16.0f);

        with([&] {
            strokeJoin(StrokeJoin::round);

            fill(lerpColor(rgba(26, 30, 38), accentColor, 0.12f));
            stroke(accentColor);
            strokeWeight(3.0f);
            hexagon(1.0f, degrees(0.0f));

            noFill();
            stroke(withOpacity(accentColor, 0.35f));
            strokeWeight(1.5f);
            hexagon(0.62f, radians(static_cast<float>(getGlobalTime()) * 0.25f));

            noStroke();
            fill(accentColor);
            circle(position.x, position.y, radius * 0.18f);
        });
    }

    void show_title(const rect2f& row, const Profile& profile)
    {
        const float2 markCenter {row.left + markRadius, center(row).y};
        show_mark(markCenter, markRadius);

        const float left = markCenter.x + markRadius + 16.0f;

        font_bold();
        textSize(26.0f);
        textLetterSpacing(1.5f);
        textAlign(TextAlignment::bottomLeft);
        fill(textColor);
        text("THE TOWER", left, center(row).y + 2.0f);
        textLetterSpacing(0.0f);

        font_medium();
        textSize(13.0f);
        textAlign(TextAlignment::topLeft);
        fill(mutedTextColor);
        text("Dauerhafte Upgrades", left, center(row).y + 6.0f);

        // the balance is the one number the whole screen is about, so it gets the same chip as a price
        font_bold();
        textSize(18.0f);
        const std::string coins = std::format("{} C", profile.coins);
        const rect2f chipRect {row.left + row.width - textWidth(coins) - 32.0f, center(row).y - 17.0f, textWidth(coins) + 32.0f, 34.0f};

        chip(chipRect, coinColor, 0.45f);
        textAlign(TextAlignment::center);
        fill(coinColor);
        text(coins, center(chipRect).x, center(chipRect).y);
    }

    void show_stat(const rect2f& tile, const std::string_view label, const std::string& value, const color_t color)
    {
        surface(tile, surfaceColor, withOpacity(borderColor, 0.3f), 10.0f);

        font_medium();
        textSize(11.0f);
        textLetterSpacing(1.0f);
        textAlign(TextAlignment::topLeft);
        fill(mutedTextColor);
        text(label, tile.left + 13.0f, tile.top + 11.0f);
        textLetterSpacing(0.0f);

        font_bold();
        textSize(20.0f);
        fill(color);
        text(value, tile.left + 13.0f, tile.top + 27.0f);
    }

    void show_stats(const rect2f& row, const Profile& profile)
    {
        constexpr int count = 3;

        show_stat(column(row, 0, count, gap), "BESTE WELLE", std::format("{}", profile.bestWave), textColor);
        show_stat(column(row, 1, count, gap), "LETZTE RUNDE", profile.lastWave > 0 ? std::format("Welle {}", profile.lastWave) : std::string("-"), profile.lastWave > 0 ? textColor : mutedTextColor);
        show_stat(column(row, 2, count, gap), "ZULETZT VERDIENT", profile.lastWave > 0 ? std::format("+{} C", profile.lastCoins) : std::string("-"), profile.lastWave > 0 ? coinColor : mutedTextColor);
    }
}

Menu menu_create(const ui::Fonts& fonts)
{
    return Menu {
        .fonts = fonts,
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

    if (clicked(l.bestiaryButton)) {
        return MenuAction::openBestiary;
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
    const rect2f screen {0.0f, 0.0f, getWidth(), getHeight()};

    arena_backdrop_show(screen, float2 {center(l.column).x, center(l.title).y + statHeight}, std::min(screen.width, screen.height) * 0.95f);

    with([&] {
        set_fonts(menu.fonts);
        noStroke();

        show_title(l.title, profile);
        show_stats(l.stats, profile);
        button(l.startButton, "Runde starten", accentColor);
        quiet_button(l.bestiaryButton, "Gegner");

        upgrade_tabs_show(l.tabs, menu.activeCategory);
        upgrade_list_show(layout_upgrades(menu, l.list), l.list, menu.scroll, [&](const UpgradeDefinition& definition) {
            return describe_line(definition, profile);
        });
    });
}
