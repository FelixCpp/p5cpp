#include "ui.hpp"

#include <algorithm>

using namespace p5;

namespace ui
{
    namespace
    {
        constexpr float pipHeight = 4.0f;
        constexpr float pipGap = 3.0f;
        constexpr float scrollSpeed = 30.0f;
    }

    bool contains(const Rect& rect, const float2& point)
    {
        return point.x >= rect.x && point.x <= rect.x + rect.w && point.y >= rect.y && point.y <= rect.y + rect.h;
    }

    float2 mouse()
    {
        return float2 {static_cast<float>(getMouseX()), static_cast<float>(getMouseY())};
    }

    bool clicked(const Rect& rect)
    {
        return isMouseButtonPressed(MouseButton::left) && contains(rect, mouse());
    }

    void tab(const Rect& rect, const std::string_view label, const bool active)
    {
        const bool hovered = contains(rect, mouse());

        noStroke();
        fill(active ? accentColor : (hovered ? buttonHoverColor : buttonColor));
        p5::rect(rect.x, rect.y, rect.w, rect.h, BorderRadius::all(6.0f));

        textSize(12.0f);
        textAlign(TextAlignment::center);
        fill(active ? panelColor : textColor);
        text(label, rect.x + rect.w * 0.5f, rect.y + rect.h * 0.5f);
    }

    void button(const Rect& rect, const std::string_view label, const color_t color)
    {
        const bool hovered = contains(rect, mouse());

        noStroke();
        fill(hovered ? lerpColor(color, rgba(255), 0.15f) : color);
        p5::rect(rect.x, rect.y, rect.w, rect.h, BorderRadius::all(10.0f));

        textSize(18.0f);
        textAlign(TextAlignment::center);
        fill(panelColor);
        text(label, rect.x + rect.w * 0.5f, rect.y + rect.h * 0.5f);
    }

    void bar(const Rect& rect, const float fraction, const color_t color)
    {
        noStroke();
        fill(buttonColor);
        p5::rect(rect.x, rect.y, rect.w, rect.h, BorderRadius::all(rect.h * 0.5f));
        if (fraction > 0.0f) {
            fill(color);
            p5::rect(rect.x, rect.y, std::max(rect.w * std::min(fraction, 1.0f), rect.h), rect.h, BorderRadius::all(rect.h * 0.5f));
        }
    }

    void upgrade_row(const Rect& rect, const UpgradeRow& row)
    {
        const bool hovered = row.affordable && contains(rect, mouse());

        fill(hovered ? buttonHoverColor : buttonColor);
        if (row.affordable) {
            stroke(accentColor);
            strokeWeight(2.0f);
        } else {
            noStroke();
        }
        p5::rect(rect.x, rect.y, rect.w, rect.h, BorderRadius::all(8.0f));
        noStroke();

        const float left = rect.x + 12.0f;
        const float right = rect.x + rect.w - 12.0f;

        textSize(15.0f);
        textAlign(TextAlignment::topLeft);
        fill(textColor);
        text(row.name, left, rect.y + 9.0f);

        textSize(12.0f);
        fill(mutedTextColor);
        text(row.detail, left, rect.y + 30.0f);

        // one segment per level, only for upgrades that have a limit
        if (row.maxLevel > 0) {
            const float pipsWidth = rect.w - 24.0f - 60.0f;
            const float pipWidth = (pipsWidth - pipGap * static_cast<float>(row.maxLevel - 1)) / static_cast<float>(row.maxLevel);
            const float pipY = rect.y + rect.h - 12.0f;

            for (int i = 0; i < row.maxLevel; ++i) {
                fill(i < row.level ? (row.maxed ? goldColor : accentColor) : panelColor);
                p5::rect(left + static_cast<float>(i) * (pipWidth + pipGap), pipY, pipWidth, pipHeight, BorderRadius::all(pipHeight * 0.5f));
            }
        }

        textSize(15.0f);
        textAlign(TextAlignment::centerRight);
        fill(row.affordable ? row.priceColor : mutedTextColor);
        text(row.price, right, rect.y + rect.h * 0.5f);
    }

    void scroll_list_update(ScrollList& list, const Rect& view, const float contentHeight)
    {
        if (contains(view, mouse())) {
            list.offset -= static_cast<float>(getScrollY()) * scrollSpeed;
        }
        list.offset = std::clamp(list.offset, 0.0f, std::max(contentHeight - view.h, 0.0f));
    }

    void scroll_list_show_bar(const ScrollList& list, const Rect& view, const float contentHeight)
    {
        if (contentHeight <= view.h || view.h <= 0.0f) {
            return;
        }

        const float thumbHeight = std::max(view.h * view.h / contentHeight, 20.0f);
        const float thumbY = view.y + (view.h - thumbHeight) * (list.offset / (contentHeight - view.h));

        noStroke();
        fill(buttonHoverColor);
        p5::rect(view.x + view.w + 4.0f, thumbY, 3.0f, thumbHeight, BorderRadius::all(1.5f));
    }
} // namespace ui
