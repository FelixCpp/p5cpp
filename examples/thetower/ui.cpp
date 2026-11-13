#include "ui.hpp"

#include <algorithm>
#include <optional>

using namespace p5;

namespace ui
{
    namespace
    {
        constexpr float scrollSpeed = 30.0f;
        constexpr float glowLayers = 9.0f;

        std::optional<Fonts> fonts;

        // only the top corners, for the highlight that sits along the lit edge of a button
        constexpr BorderRadius topCorners(const float radius)
        {
            return BorderRadius {
                .topLeft = CornerRadius::circular(radius),
                .topRight = CornerRadius::circular(radius),
                .bottomRight = CornerRadius::circular(0.0f),
                .bottomLeft = CornerRadius::circular(0.0f),
            };
        }
    }

    void set_fonts(const Fonts& used)
    {
        fonts = used;
        font_medium();
    }

    void font_bold()
    {
        if (fonts.has_value()) {
            textFont(fonts->bold);
        }
    }

    void font_medium()
    {
        if (fonts.has_value()) {
            textFont(fonts->medium);
        }
    }

    float2 mouse()
    {
        return float2 {static_cast<float>(getMouseX()), static_cast<float>(getMouseY())};
    }

    bool clicked(const rect2f& rect)
    {
        return isMouseButtonPressed(MouseButton::left) && contains(rect, mouse());
    }

    void glow(const rect2f& rect, const color_t color, const float strength, const float radius, const float spread)
    {
        if (strength <= 0.0f) {
            return;
        }

        with([&] {
            blendMode(BlendMode::additive);
            noStroke();
            fill(withOpacity(color, 0.032f * strength));

            for (float i = glowLayers; i >= 1.0f; --i) {
                const float grown = spread * i / glowLayers;
                p5::rect(rect.left - grown, rect.top - grown, rect.width + grown * 2.0f, rect.height + grown * 2.0f, BorderRadius::all(radius + grown));
            }
        });
    }

    void surface(const rect2f& rect, const color_t background, const color_t edge, const float radius)
    {
        noStroke();
        fill(background);
        p5::rect(rect.left, rect.top, rect.width, rect.height, BorderRadius::all(radius));

        noFill();
        stroke(edge);
        strokeWeight(1.0f);
        p5::rect(rect.left + 0.5f, rect.top + 0.5f, rect.width - 1.0f, rect.height - 1.0f, BorderRadius::all(radius));
        noStroke();
    }

    void chip(const rect2f& rect, const color_t color, const float emphasis)
    {
        glow(rect, color, emphasis, rect.height * 0.35f, 8.0f);
        surface(rect, withOpacity(color, 0.12f + 0.1f * emphasis), withOpacity(color, 0.45f + 0.35f * emphasis), rect.height * 0.35f);
    }

    void hairline(const rect2f& rect)
    {
        noStroke();
        fill(withOpacity(borderColor, 0.55f));
        p5::rect(rect.left, rect.top, rect.width, 1.0f);
    }

    void tab(const rect2f& rect, const std::string_view label, const bool active)
    {
        const bool hovered = contains(rect, mouse());

        if (active) {
            chip(rect, accentColor, 1.0f);
        } else {
            surface(rect, hovered ? buttonHoverColor : buttonColor, withOpacity(borderColor, hovered ? 0.55f : 0.25f), rect.height * 0.35f);
        }

        active ? font_bold() : font_medium();
        textSize(14.0f);
        textAlign(TextAlignment::center);
        fill(active ? accentColor : (hovered ? textColor : secondaryTextColor));
        text(label, center(rect).x, center(rect).y);
    }

    void button(const rect2f& rect, const std::string_view label, const color_t color)
    {
        constexpr float radius = 12.0f;
        const bool hovered = contains(rect, mouse());

        glow(rect, color, hovered ? 1.6f : 1.0f, radius, 16.0f);

        noStroke();
        fill(hovered ? lerpColor(color, rgba(255), 0.18f) : color);
        p5::rect(rect.left, rect.top, rect.width, rect.height, BorderRadius::all(radius));

        // a brighter band along the top edge, so the fill looks lit from above instead of flat
        fill(withOpacity(rgba(255), 0.1f));
        p5::rect(rect.left + 1.5f, rect.top + 1.5f, rect.width - 3.0f, rect.height * 0.42f, topCorners(radius - 1.5f));

        font_bold();
        textSize(17.0f);
        textAlign(TextAlignment::center);
        fill(panelColor);
        text(label, center(rect).x, center(rect).y);
    }

    void bar(const rect2f& rect, const float fraction, const color_t color)
    {
        const float radius = rect.height * 0.5f;

        noStroke();
        fill(surfaceColor);
        p5::rect(rect.left, rect.top, rect.width, rect.height, BorderRadius::all(radius));

        if (fraction <= 0.0f) {
            return;
        }

        const float filled = std::max(rect.width * std::min(fraction, 1.0f), rect.height);

        glow(rect2f {rect.left, rect.top, filled, rect.height}, color, 0.9f, radius, 7.0f);

        noStroke();
        fill(color);
        p5::rect(rect.left, rect.top, filled, rect.height, BorderRadius::all(radius));

        // the leading edge keeps a brighter bead, so a bar that is still filling reads as moving
        if (fraction < 1.0f) {
            fill(withOpacity(rgba(255), 0.5f));
            p5::rect(rect.left + filled - rect.height, rect.top, rect.height, rect.height, BorderRadius::all(radius));
        }
    }

    void card(const rect2f& rect, const std::string_view title)
    {
        surface(rect, surfaceColor, withOpacity(borderColor, 0.3f), 10.0f);

        // the group is marked by a tick in the colour of the tower rather than by a coloured headline
        noStroke();
        fill(accentColor);
        p5::rect(rect.left + 12.0f, rect.top + 12.0f, 3.0f, 13.0f, BorderRadius::all(1.5f));

        font_medium();
        textSize(12.0f);
        textLetterSpacing(0.8f);
        textAlign(TextAlignment::topLeft);
        fill(secondaryTextColor);
        text(title, rect.left + 22.0f, rect.top + 11.0f);
        textLetterSpacing(0.0f);
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

        const float thumbHeight = std::max(view.height * view.height / contentHeight, 24.0f);
        const float thumbY = view.top + (view.height - thumbHeight) * (list.offset / (contentHeight - view.height));

        noStroke();
        fill(withOpacity(borderColor, 0.25f));
        p5::rect(view.left + view.width + 5.0f, view.top, 3.0f, view.height, BorderRadius::all(1.5f));
        fill(withOpacity(accentColor, 0.55f));
        p5::rect(view.left + view.width + 5.0f, thumbY, 3.0f, thumbHeight, BorderRadius::all(1.5f));
    }
} // namespace ui
