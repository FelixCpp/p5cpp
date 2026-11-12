#pragma once

#include <string>
#include <string_view>

#include <p5cpp/p5cpp.hpp>

namespace ui
{
    inline constexpr p5::color_t panelColor = p5::rgba(24);
    inline constexpr p5::color_t buttonColor = p5::rgba(38);
    inline constexpr p5::color_t buttonHoverColor = p5::rgba(50);
    inline constexpr p5::color_t accentColor = p5::rgba(60, 180, 220);
    inline constexpr p5::color_t goldColor = p5::rgba(230, 200, 90);
    inline constexpr p5::color_t coinColor = p5::rgba(180, 140, 240);
    inline constexpr p5::color_t textColor = p5::rgba(235);
    inline constexpr p5::color_t secondaryTextColor = p5::rgba(180); // values and labels that still need to be read
    inline constexpr p5::color_t mutedTextColor = p5::rgba(150);     // hints, inactive states
    inline constexpr p5::color_t healthColor = p5::rgba(110, 200, 120);
    inline constexpr p5::color_t dangerColor = p5::rgba(230, 80, 80);

    inline constexpr float tabHeight = 32.0f;
    inline constexpr float cardHeaderHeight = 32.0f;
    inline constexpr float cardGap = 10.0f;
    inline constexpr float upgradeLineHeight = 52.0f;

    struct Rect
    {
        float x, y, w, h;
    };

    bool contains(const Rect& rect, const p5::float2& point);
    p5::float2 mouse();
    bool clicked(const Rect& rect);

    void tab(const Rect& rect, std::string_view label, bool active);
    void button(const Rect& rect, std::string_view label, p5::color_t color);
    void bar(const Rect& rect, float fraction, p5::color_t color);

    // a titled box that groups upgrade lines
    void card(const Rect& rect, std::string_view title);

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

    void upgrade_line(const Rect& rect, const UpgradeLine& line, bool divider);

    // vertical list that scrolls with the mouse wheel while hovered
    struct ScrollList
    {
        float offset;
    };

    void scroll_list_update(ScrollList& list, const Rect& view, float contentHeight);
    void scroll_list_show_bar(const ScrollList& list, const Rect& view, float contentHeight);
} // namespace ui
