#pragma once

#include <string_view>

#include <p5cpp/p5cpp.hpp>

#include "palette.hpp"

namespace ui
{
    // the interface borrows the blue-black of the arena, so a panel reads as part of the playfield instead of a grey box on top of it
    inline constexpr p5::color_t panelColor = p5::rgba(14, 17, 23);
    inline constexpr p5::color_t surfaceColor = p5::rgba(25, 30, 40); // cards and the tracks of bars
    inline constexpr p5::color_t buttonColor = p5::rgba(34, 41, 54);
    inline constexpr p5::color_t buttonHoverColor = p5::rgba(48, 58, 75);
    inline constexpr p5::color_t borderColor = p5::rgba(70, 88, 112); // hairlines, always drawn with an opacity below one
    inline constexpr p5::color_t textColor = p5::rgba(234, 240, 248);
    inline constexpr p5::color_t secondaryTextColor = p5::rgba(156, 172, 194); // values and labels that still need to be read
    inline constexpr p5::color_t mutedTextColor = p5::rgba(100, 116, 138);     // hints, inactive states

    inline constexpr float tabHeight = 34.0f;

    // the bold weight carries names, numbers and buttons, the medium one everything that only labels them
    struct Fonts
    {
        p5::Font bold;
        p5::Font medium;
    };

    // p5 keeps the text font in global state, so the two weights live there as well
    void set_fonts(const Fonts& fonts);
    void font_bold();
    void font_medium();

    p5::float2 mouse();
    bool clicked(const p5::rect2f& rect);

    // a soft halo around a rectangle, the same stack of translucent layers the tower glows with
    void glow(const p5::rect2f& rect, p5::color_t color, float strength, float radius, float spread = 10.0f);

    // a filled rectangle with a hairline edge, the edge is what lifts it off the panel
    void surface(const p5::rect2f& rect, p5::color_t background, p5::color_t edge, float radius);

    // the surface of something that is shown in its own color: a balance, a price, a boss warning
    void chip(const p5::rect2f& rect, p5::color_t color, float emphasis);

    void hairline(const p5::rect2f& rect);

    void tab(const p5::rect2f& rect, std::string_view label, bool active);
    void button(const p5::rect2f& rect, std::string_view label, p5::color_t color);
    // a button that does not compete with the main action beside it
    void quiet_button(const p5::rect2f& rect, std::string_view label);
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
