#include "upgrade_list.hpp"

using namespace p5;
using namespace ui;

namespace
{
    constexpr float linePadding = 4.0f;
}

UpgradeListLayout upgrade_list_layout(const UpgradeCategory category, const Rect& view, const float scrollOffset)
{
    UpgradeListLayout layout = {};
    const float top = view.y - scrollOffset;
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
                .rect = Rect {view.x, y, view.w, 0.0f},
            });
            y += cardHeaderHeight;
        }

        layout.lines.push_back(UpgradeListLine {
            .definition = &definition,
            .rect = Rect {view.x + linePadding, y, view.w - linePadding * 2.0f, upgradeLineHeight},
            .firstInCard = newCard,
        });
        y += upgradeLineHeight;

        UpgradeListCard& card = layout.cards.back();
        card.rect.h = y + linePadding - card.rect.y;
    }

    layout.contentHeight = layout.cards.empty() ? 0.0f : y + linePadding - top;
    return layout;
}

const UpgradeDefinition* upgrade_list_clicked(const UpgradeListLayout& layout, const Rect& view)
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

void upgrade_list_show(const UpgradeListLayout& layout, const Rect& view, const ScrollList& scroll, const std::function<UpgradeLine(const UpgradeDefinition&)>& describe)
{
    withClip(view.x - 2.0f, view.y, view.w + 4.0f, view.h, [&] {
        for (const UpgradeListCard& card : layout.cards) {
            ui::card(card.rect, card.group);
        }
        for (const UpgradeListLine& line : layout.lines) {
            upgrade_line(line.rect, describe(*line.definition), !line.firstInCard);
        }
    });
    scroll_list_show_bar(scroll, view, layout.contentHeight);
}
