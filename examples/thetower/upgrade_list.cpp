#include "upgrade_list.hpp"

#include <algorithm>
#include <array>
#include <format>

using namespace p5;
using namespace ui;

namespace
{
    constexpr std::array categories = {UpgradeCategory::attack, UpgradeCategory::defense, UpgradeCategory::utility};
    constexpr float tabGap = 8.0f;

    constexpr float cardHeaderHeight = 34.0f;
    constexpr float cardGap = 12.0f;
    constexpr float lineHeight = 56.0f;
    constexpr float linePadding = 5.0f;

    constexpr float pipHeight = 4.0f;
    constexpr float pipGap = 3.0f;
    constexpr float pipsWidth = 170.0f;
    constexpr float priceWidth = 70.0f;
    constexpr float priceHeight = 32.0f;
    constexpr float priceGap = 10.0f;

    // the arrow has no glyph in the font, so it is drawn
    void show_arrow(const float x, const float y)
    {
        stroke(withOpacity(accentColor, 0.65f));
        strokeWeight(1.5f);
        strokeCap(StrokeCap::round);
        p5::line(x, y, x + 10.0f, y);
        p5::line(x + 6.0f, y - 3.5f, x + 10.0f, y);
        p5::line(x + 6.0f, y + 3.5f, x + 10.0f, y);
        noStroke();
    }

    // one segment per level, only for upgrades that have a limit
    void show_pips(rect2f row, const UpgradeLine& line)
    {
        row.width = std::min(row.width, pipsWidth);
        const float pipWidth = (row.width - pipGap * static_cast<float>(line.maxLevel - 1)) / static_cast<float>(line.maxLevel);
        const color_t reached = line.maxed ? goldColor : accentColor;

        for (int i = 0; i < line.maxLevel; ++i) {
            const rect2f pip {row.left + static_cast<float>(i) * (pipWidth + pipGap), row.top, pipWidth, row.height};

            if (i < line.level) {
                glow(pip, reached, 0.7f, pipHeight * 0.5f, 4.0f);
                fill(reached);
            } else {
                fill(withOpacity(borderColor, 0.35f));
            }
            p5::rect(pip.left, pip.top, pip.width, pip.height, BorderRadius::all(pipHeight * 0.5f));
        }
    }

    void show_price(const rect2f& price, const UpgradeLine& line, const bool hovered)
    {
        if (line.maxed) {
            chip(price, goldColor, 0.0f);
        } else if (line.affordable) {
            chip(price, line.priceColor, hovered ? 1.0f : 0.35f);
        } else {
            surface(price, panelColor, withOpacity(borderColor, 0.2f), price.height * 0.35f);
        }

        font_bold();
        textSize(14.0f);
        textAlign(TextAlignment::center);
        fill(line.maxed ? goldColor : (line.affordable ? line.priceColor : mutedTextColor));
        text(line.price, center(price).x, center(price).y);
    }

    void show_line(const rect2f& row, const UpgradeLine& line, const bool divider)
    {
        const bool hovered = line.affordable && contains(row, mouse());

        if (hovered) {
            glow(row, accentColor, 0.5f, 8.0f, 6.0f);
            surface(row, buttonColor, withOpacity(accentColor, 0.35f), 8.0f);
        }

        rect2f content = inset(row, 10.0f, 0.0f);
        if (divider && !hovered) {
            hairline(content);
        }

        const rect2f price = inset(cutRight(content, priceWidth), 0.0f, (row.height - priceHeight) * 0.5f);
        cutRight(content, priceGap);
        show_price(price, line, hovered);

        cutTop(content, 8.0f);
        const rect2f nameRow = cutTop(content, 17.0f);
        const rect2f valueRow = cutTop(content, 16.0f);

        font_bold();
        textSize(15.0f);
        textAlign(TextAlignment::centerLeft);
        fill(textColor);
        text(line.name, nameRow.left, center(nameRow).y);
        const float nameWidth = textWidth(line.name);

        font_medium();
        textSize(11.0f);
        fill(mutedTextColor);
        text(line.levelLabel, nameRow.left + nameWidth + 8.0f, center(nameRow).y);

        // the current value, an arrow, and the value the next level would bring in the colour of the tower
        const float valueY = center(valueRow).y;
        textSize(13.0f);
        fill(secondaryTextColor);
        text(line.value, valueRow.left, valueY);
        const float valueWidth = textWidth(line.value);

        if (!line.nextValue.empty()) {
            const float arrowX = valueRow.left + valueWidth + 7.0f;
            show_arrow(arrowX, valueY);

            font_bold();
            fill(accentColor);
            text(line.nextValue, arrowX + 17.0f, valueY);
        }

        if (line.maxLevel > 0) {
            cutTop(content, 6.0f);
            show_pips(cutTop(content, pipHeight), line);
        }
    }
}

std::vector<UpgradeTab> upgrade_tabs_layout(const rect2f& area)
{
    constexpr int count = static_cast<int>(categories.size());

    std::vector<UpgradeTab> tabs;
    for (int i = 0; i < count; ++i) {
        tabs.push_back(UpgradeTab {
            .category = categories[static_cast<size_t>(i)],
            .rect = column(area, i, count, tabGap),
        });
    }
    return tabs;
}

std::optional<UpgradeCategory> upgrade_tabs_clicked(const std::vector<UpgradeTab>& tabs)
{
    for (const UpgradeTab& tab : tabs) {
        if (clicked(tab.rect)) {
            return tab.category;
        }
    }
    return std::nullopt;
}

void upgrade_tabs_show(const std::vector<UpgradeTab>& tabs, const UpgradeCategory active)
{
    for (const UpgradeTab& tab : tabs) {
        ui::tab(tab.rect, upgrade_category_name(tab.category), tab.category == active);
    }
}

UpgradeLine upgrade_line_describe(const UpgradeDefinition& definition, const TowerStats& baseStats, const UpgradeLevels& levels, const std::string_view levelLabel, const int price, const UpgradeCurrency& currency)
{
    const int level = upgrade_level(levels, definition.id);
    const bool maxed = upgrade_is_maxed(definition, level);

    UpgradeLevels nextLevels = levels;
    ++nextLevels.levels[static_cast<size_t>(definition.id)];

    return UpgradeLine {
        .name = definition.name,
        .levelLabel = std::format("{} {}", levelLabel, level),
        .value = definition.describe(tower_stats_compute(baseStats, levels)),
        .nextValue = maxed ? std::string() : definition.describe(tower_stats_compute(baseStats, nextLevels)),
        .level = level,
        .maxLevel = definition.maxLevel,
        .price = maxed ? std::string("MAX") : std::format("{} {}", price, currency.symbol),
        .priceColor = currency.color,
        .affordable = !maxed && currency.balance >= price,
        .maxed = maxed,
    };
}

UpgradeListLayout upgrade_list_layout(const UpgradeCategory category, const rect2f& view, const float scrollOffset)
{
    UpgradeListLayout layout = {};
    const float top = view.top - scrollOffset;
    float y = top;

    for (const UpgradeDefinition& definition : upgrade_definitions()) {
        if (definition.category != category) {
            continue;
        }

        const bool newCard = layout.cards.empty() || layout.cards.back().group != definition.group;
        if (newCard) {
            if (!layout.cards.empty()) {
                y += linePadding + cardGap;
            }
            layout.cards.push_back(UpgradeListCard {
                .group = definition.group,
                .rect = rect2f {view.left, y, view.width, 0.0f},
            });
            y += cardHeaderHeight;
        }

        layout.lines.push_back(UpgradeListLine {
            .definition = &definition,
            .rect = rect2f {view.left + linePadding, y, view.width - linePadding * 2.0f, lineHeight},
            .firstInCard = newCard,
        });
        y += lineHeight;

        UpgradeListCard& card = layout.cards.back();
        card.rect.height = y + linePadding - card.rect.top;
    }

    layout.contentHeight = layout.cards.empty() ? 0.0f : y + linePadding - top;
    return layout;
}

const UpgradeDefinition* upgrade_list_clicked(const UpgradeListLayout& layout, const rect2f& view)
{
    if (!clicked(view)) {
        return nullptr;
    }
    for (const UpgradeListLine& line : layout.lines) {
        if (clicked(line.rect)) {
            return line.definition;
        }
    }
    return nullptr;
}

void upgrade_list_show(const UpgradeListLayout& layout, const rect2f& view, const ScrollList& scroll, const std::function<UpgradeLine(const UpgradeDefinition&)>& describe)
{
    withClip(view.left - 2.0f, view.top, view.width + 4.0f, view.height, [&] {
        for (const UpgradeListCard& card : layout.cards) {
            ui::card(card.rect, card.group);
        }
        for (const UpgradeListLine& line : layout.lines) {
            show_line(line.rect, describe(*line.definition), !line.firstInCard);
        }
    });
    scroll_list_show_bar(scroll, view, layout.contentHeight);
}
