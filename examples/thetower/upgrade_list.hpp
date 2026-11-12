#pragma once

#include <functional>
#include <vector>

#include "ui.hpp"
#include "upgrades.hpp"

// the upgrades of one category, one card per group, shared by the round hud and the menu

struct UpgradeListCard
{
    std::string_view group;
    ui::Rect rect;
};

struct UpgradeListLine
{
    const UpgradeDefinition* definition;
    ui::Rect rect;
    bool firstInCard;
};

struct UpgradeListLayout
{
    std::vector<UpgradeListCard> cards;
    std::vector<UpgradeListLine> lines;
    float contentHeight;
};

UpgradeListLayout upgrade_list_layout(UpgradeCategory category, const ui::Rect& view, float scrollOffset);

// the clicked line, lines scrolled out of the view never count
const UpgradeDefinition* upgrade_list_clicked(const UpgradeListLayout& layout, const ui::Rect& view);

void upgrade_list_show(const UpgradeListLayout& layout, const ui::Rect& view, const ui::ScrollList& scroll, const std::function<ui::UpgradeLine(const UpgradeDefinition&)>& describe);
