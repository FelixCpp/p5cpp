#include "ui.hpp"

#include <algorithm>

using namespace p5;

namespace ui
{
    namespace
    {
        constexpr float pipHeight = 3.0f;
        constexpr float pipGap = 3.0f;
        constexpr float priceWidth = 68.0f;
        constexpr float priceHeight = 30.0f;
        constexpr color_t emptyPipColor = rgba(62);
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

        textSize(14.0f);
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

    void card(const Rect& rect, const std::string_view title)
    {
        noStroke();
        fill(buttonColor);
        p5::rect(rect.x, rect.y, rect.w, rect.h, BorderRadius::all(10.0f));

        textSize(13.0f);
        textAlign(TextAlignment::topLeft);
        fill(accentColor);
        text(title, rect.x + 12.0f, rect.y + 10.0f);
    }

    void upgrade_line(const Rect& rect, const UpgradeLine& line, const bool divider)
    {
        const bool hovered = line.affordable && contains(rect, mouse());

        noStroke();
        if (hovered) {
            fill(buttonHoverColor);
            p5::rect(rect.x, rect.y, rect.w, rect.h, BorderRadius::all(6.0f));
        } else if (divider) {
            fill(panelColor);
            p5::rect(rect.x + 8.0f, rect.y, rect.w - 16.0f, 1.0f);
        }

        const float left = rect.x + 8.0f;
        const float right = rect.x + rect.w - 8.0f;
        const Rect price = {right - priceWidth, rect.y + (rect.h - priceHeight) * 0.5f, priceWidth, priceHeight};

        textSize(15.0f);
        textAlign(TextAlignment::topLeft);
        fill(textColor);
        text(line.name, left, rect.y + 8.0f);
        const float nameWidth = textWidth(line.name);

        textSize(12.0f);
        fill(mutedTextColor);
        text(line.levelLabel, left + nameWidth + 8.0f, rect.y + 10.0f);

        // current value, a drawn arrow (the font has no glyph for it) and the highlighted next value
        const float valueY = rect.y + 28.0f;
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
            const float pipsWidth = price.x - 12.0f - left;
            const float pipWidth = (pipsWidth - pipGap * static_cast<float>(line.maxLevel - 1)) / static_cast<float>(line.maxLevel);
            const float pipY = rect.y + rect.h - 7.0f;

            for (int i = 0; i < line.maxLevel; ++i) {
                fill(i < line.level ? (line.maxed ? goldColor : accentColor) : emptyPipColor);
                p5::rect(left + static_cast<float>(i) * (pipWidth + pipGap), pipY, pipWidth, pipHeight, BorderRadius::all(pipHeight * 0.5f));
            }
        }

        if (line.maxed) {
            textSize(14.0f);
            textAlign(TextAlignment::center);
            fill(goldColor);
            text("MAX", price.x + price.w * 0.5f, price.y + price.h * 0.5f);
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
        p5::rect(price.x, price.y, price.w, price.h, BorderRadius::all(8.0f));
        noStroke();

        textSize(14.0f);
        textAlign(TextAlignment::center);
        fill(line.affordable ? line.priceColor : mutedTextColor);
        text(line.price, price.x + price.w * 0.5f, price.y + price.h * 0.5f);
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
