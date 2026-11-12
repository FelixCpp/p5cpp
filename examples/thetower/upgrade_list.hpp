#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ui.hpp"
#include "upgrades.hpp"

// the upgrade panel shared by the round hud and the menu: one tab per category,
// below it the upgrades of the active category, one card per group

// the tabs are spread evenly over the row
struct UpgradeTab
{
    UpgradeCategory category;
    p5::rect2f rect;
};

std::vector<UpgradeTab> upgrade_tabs_layout(const p5::rect2f& row);
std::optional<UpgradeCategory> upgrade_tabs_clicked(const std::vector<UpgradeTab>& tabs);
void upgrade_tabs_show(const std::vector<UpgradeTab>& tabs, UpgradeCategory active);

struct UpgradeLine
{
    std::string_view name;
    std::string levelLabel;
    std::string value;
    std::string nextValue; // empty when there is no next level
    int level;
    int maxLevel; // 0 = no pips
    std::string price;
    p5::color_t priceColor;
    bool affordable;
    bool maxed;
};

// what the list charges in: gold during a round, coins in the menu
struct UpgradeCurrency
{
    std::string_view symbol;
    p5::color_t color;
    int balance;
};

// the line for an upgrade at the given levels, price is what its next level costs
UpgradeLine upgrade_line_describe(const UpgradeDefinition& definition, const TowerStats& baseStats, const UpgradeLevels& levels, std::string_view levelLabel, int price, const UpgradeCurrency& currency);

struct UpgradeListCard
{
    std::string_view group;
    p5::rect2f rect;
};

struct UpgradeListLine
{
    const UpgradeDefinition* definition;
    p5::rect2f rect;
    bool firstInCard;
};

struct UpgradeListLayout
{
    std::vector<UpgradeListCard> cards;
    std::vector<UpgradeListLine> lines;
    float contentHeight;
};

UpgradeListLayout upgrade_list_layout(UpgradeCategory category, const p5::rect2f& view, float scrollOffset);

// the clicked line, lines scrolled out of the view never count
const UpgradeDefinition* upgrade_list_clicked(const UpgradeListLayout& layout, const p5::rect2f& view);

void upgrade_list_show(const UpgradeListLayout& layout, const p5::rect2f& view, const ui::ScrollList& scroll, const std::function<UpgradeLine(const UpgradeDefinition&)>& describe);
