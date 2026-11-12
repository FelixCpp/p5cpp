#include "upgrade_list.hpp"

#include <array>
#include <format>

using namespace p5;
using namespace ui;

namespace
{
    constexpr std::array categories = {UpgradeCategory::attack, UpgradeCategory::defense, UpgradeCategory::utility};
    constexpr float tabGap = 8.0f;

    constexpr float cardHeaderHeight = 32.0f;
    constexpr float cardGap = 10.0f;
    constexpr float lineHeight = 52.0f;
    constexpr float linePadding = 4.0f;

    constexpr float pipHeight = 3.0f;
    constexpr float pipGap = 3.0f;
    constexpr float priceWidth = 68.0f;
    constexpr float priceHeight = 30.0f;
    constexpr color_t emptyPipColor = rgba(62);

    void show_line(const rect2f& rect, const UpgradeLine& line, const bool divider)
    {
        const bool hovered = line.affordable && contains(rect, mouse());

        noStroke();
        if (hovered) {
            fill(buttonHoverColor);
            p5::rect(rect.left, rect.top, rect.width, rect.height, BorderRadius::all(6.0f));
        } else if (divider) {
            fill(panelColor);
            p5::rect(rect.left + 8.0f, rect.top, rect.width - 16.0f, 1.0f);
        }

        const float left = rect.left + 8.0f;
        const float right = rect.left + rect.width - 8.0f;
        const rect2f price = {right - priceWidth, rect.top + (rect.height - priceHeight) * 0.5f, priceWidth, priceHeight};

        textSize(15.0f);
        textAlign(TextAlignment::topLeft);
        fill(textColor);
        text(line.name, left, rect.top + 8.0f);
        const float nameWidth = textWidth(line.name);

        textSize(12.0f);
        fill(mutedTextColor);
        text(line.levelLabel, left + nameWidth + 8.0f, rect.top + 10.0f);

        // current value, a drawn arrow (the font has no glyph for it) and the highlighted next value
        const float valueY = rect.top + 28.0f;
        textSize(13.0f);
        fill(secondaryTextColor);
        text(line.value, left, valueY);
        if (!line.nextValue.empty()) {
            const float arrowX = left + textWidth(line.value) + 6.0f;
            const float arrowY = valueY + 8.0f;

            stroke(mutedTextColor);
            strokeWeight(1.5f);
            p5::line(arrowX, arrowY, arrowX + 10.0f, arrowY);
            p5::line(arrowX + 6.0f, arrowY - 3.5f, arrowX + 10.0f, arrowY);
            p5::line(arrowX + 6.0f, arrowY + 3.5f, arrowX + 10.0f, arrowY);
            noStroke();

            fill(textColor);
            text(line.nextValue, arrowX + 16.0f, valueY);
        }

        // one segment per level, only for upgrades that have a limit
        if (line.maxLevel > 0) {
            const float pipsWidth = price.left - 12.0f - left;
            const float pipWidth = (pipsWidth - pipGap * static_cast<float>(line.maxLevel - 1)) / static_cast<float>(line.maxLevel);
            const float pipY = rect.top + rect.height - 7.0f;

            for (int i = 0; i < line.maxLevel; ++i) {
                fill(i < line.level ? (line.maxed ? goldColor : accentColor) : emptyPipColor);
                p5::rect(left + static_cast<float>(i) * (pipWidth + pipGap), pipY, pipWidth, pipHeight, BorderRadius::all(pipHeight * 0.5f));
            }
        }

        if (line.maxed) {
            textSize(14.0f);
            textAlign(TextAlignment::center);
            fill(goldColor);
            text("MAX", price.left + price.width * 0.5f, price.top + price.height * 0.5f);
            return;
        }

        if (line.affordable) {
            fill(lerpColor(buttonColor, line.priceColor, hovered ? 0.35f : 0.2f));
            stroke(line.priceColor);
            strokeWeight(1.5f);
        } else {
            fill(panelColor);
            noStroke();
        }
        p5::rect(price.left, price.top, price.width, price.height, BorderRadius::all(8.0f));
        noStroke();

        textSize(14.0f);
        textAlign(TextAlignment::center);
        fill(line.affordable ? line.priceColor : mutedTextColor);
        text(line.price, price.left + price.width * 0.5f, price.top + price.height * 0.5f);
    }
}

std::vector<UpgradeTab> upgrade_tabs_layout(const rect2f& row)
{
    const float tabWidth = (row.width - tabGap * static_cast<float>(categories.size() - 1)) / static_cast<float>(categories.size());

    std::vector<UpgradeTab> tabs;
    for (size_t i = 0; i < categories.size(); ++i) {
        tabs.push_back(UpgradeTab {
            .category = categories[i],
            .rect = rect2f {row.left + static_cast<float>(i) * (tabWidth + tabGap), row.top, tabWidth, row.height},
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
