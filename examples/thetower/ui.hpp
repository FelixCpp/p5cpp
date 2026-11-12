#pragma once

#include <string_view>

#include <p5cpp/p5cpp.hpp>

#include "palette.hpp"

namespace ui
{
    inline constexpr p5::color_t panelColor = p5::rgba(24);
    inline constexpr p5::color_t buttonColor = p5::rgba(38);
    inline constexpr p5::color_t buttonHoverColor = p5::rgba(50);
    inline constexpr p5::color_t textColor = p5::rgba(235);
    inline constexpr p5::color_t secondaryTextColor = p5::rgba(180); // values and labels that still need to be read
    inline constexpr p5::color_t mutedTextColor = p5::rgba(150);     // hints, inactive states

    inline constexpr float tabHeight = 32.0f;

    p5::float2 mouse();
    bool clicked(const p5::rect2f& rect);

    void tab(const p5::rect2f& rect, std::string_view label, bool active);
    void button(const p5::rect2f& rect, std::string_view label, p5::color_t color);
    void bar(const p5::rect2f& rect, float fraction, p5::color_t color);

    // a box with a title in its top left corner
    void card(const p5::rect2f& rect, std::string_view title);

    // vertical list that scrolls with the mouse wheel while hovered
    struct ScrollList
    {
        float offset;
    };

    void scroll_list_update(ScrollList& list, const p5::rect2f& view, float contentHeight);
    void scroll_list_show_bar(const ScrollList& list, const p5::rect2f& view, float contentHeight);
} // namespace ui
