#include "ui.hpp"

#include <algorithm>

using namespace p5;

namespace ui
{
    namespace
    {
        constexpr float scrollSpeed = 30.0f;
    }

    float2 mouse()
    {
        return float2 {static_cast<float>(getMouseX()), static_cast<float>(getMouseY())};
    }

    bool clicked(const rect2f& rect)
    {
        return isMouseButtonPressed(MouseButton::left) && contains(rect, mouse());
    }

    void tab(const rect2f& rect, const std::string_view label, const bool active)
    {
        const bool hovered = contains(rect, mouse());

        noStroke();
        fill(active ? accentColor : (hovered ? buttonHoverColor : buttonColor));
        p5::rect(rect.left, rect.top, rect.width, rect.height, BorderRadius::all(6.0f));

        textSize(14.0f);
        textAlign(TextAlignment::center);
        fill(active ? panelColor : textColor);
        text(label, rect.left + rect.width * 0.5f, rect.top + rect.height * 0.5f);
    }

    void button(const rect2f& rect, const std::string_view label, const color_t color)
    {
        const bool hovered = contains(rect, mouse());

        noStroke();
        fill(hovered ? lerpColor(color, rgba(255), 0.15f) : color);
        p5::rect(rect.left, rect.top, rect.width, rect.height, BorderRadius::all(10.0f));

        textSize(18.0f);
        textAlign(TextAlignment::center);
        fill(panelColor);
        text(label, rect.left + rect.width * 0.5f, rect.top + rect.height * 0.5f);
    }

    void bar(const rect2f& rect, const float fraction, const color_t color)
    {
        noStroke();
        fill(buttonColor);
        p5::rect(rect.left, rect.top, rect.width, rect.height, BorderRadius::all(rect.height * 0.5f));
        if (fraction > 0.0f) {
            fill(color);
            p5::rect(rect.left, rect.top, std::max(rect.width * std::min(fraction, 1.0f), rect.height), rect.height, BorderRadius::all(rect.height * 0.5f));
        }
    }

    void card(const rect2f& rect, const std::string_view title)
    {
        noStroke();
        fill(buttonColor);
        p5::rect(rect.left, rect.top, rect.width, rect.height, BorderRadius::all(10.0f));

        textSize(13.0f);
        textAlign(TextAlignment::topLeft);
        fill(accentColor);
        text(title, rect.left + 12.0f, rect.top + 10.0f);
    }

    void scroll_list_update(ScrollList& list, const rect2f& view, const float contentHeight)
    {
        if (contains(view, mouse())) {
            list.offset -= static_cast<float>(getScrollY()) * scrollSpeed;
        }
        list.offset = std::clamp(list.offset, 0.0f, std::max(contentHeight - view.height, 0.0f));
    }

    void scroll_list_show_bar(const ScrollList& list, const rect2f& view, const float contentHeight)
    {
        if (contentHeight <= view.height || view.height <= 0.0f) {
            return;
        }

        const float thumbHeight = std::max(view.height * view.height / contentHeight, 20.0f);
        const float thumbY = view.top + (view.height - thumbHeight) * (list.offset / (contentHeight - view.height));

        noStroke();
        fill(buttonHoverColor);
        p5::rect(view.left + view.width + 4.0f, thumbY, 3.0f, thumbHeight, BorderRadius::all(1.5f));
    }
} // namespace ui
