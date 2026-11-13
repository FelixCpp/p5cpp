#include "round_controls.hpp"

#include <format>

using namespace p5;
using namespace ui;

namespace
{
    constexpr float buttonSize = 32.0f;
    constexpr float speedWidth = 60.0f;
    constexpr float panelPadding = 4.0f;
    constexpr float buttonGap = 4.0f;
    constexpr float controlsWidth = buttonSize * 3.0f + speedWidth + buttonGap * 3.0f;

    bool pressed_any(const std::initializer_list<Key> keys)
    {
        for (const Key key : keys) {
            if (isKeyPressed(key)) {
                return true;
            }
        }
        return false;
    }

    void icon_button(const rect2f& rect, const bool enabled)
    {
        const bool hovered = enabled && contains(rect, mouse());

        if (hovered) {
            glow(rect, accentColor, 0.8f, 8.0f, 7.0f);
        }
        surface(rect, hovered ? buttonHoverColor : buttonColor, withOpacity(borderColor, hovered ? 0.6f : 0.25f), 8.0f);

        stroke(hovered ? accentColor : (enabled ? textColor : mutedTextColor));
        strokeWeight(2.5f);
        strokeCap(StrokeCap::round);
    }
}

RoundControlsLayout round_controls_layout(const float2& topLeft)
{
    const rect2f panel {topLeft.x, topLeft.y, controlsWidth + panelPadding * 2.0f, buttonSize + panelPadding * 2.0f};
    rect2f row = inset(panel, panelPadding);

    RoundControlsLayout layout = {};
    layout.panel = panel;
    layout.pause = cutLeft(row, buttonSize);
    cutLeft(row, buttonGap * 3.0f);
    layout.slower = cutLeft(row, buttonSize);
    layout.speed = cutLeft(row, speedWidth);
    layout.faster = cutLeft(row, buttonSize);
    return layout;
}

RoundAction round_controls_update(const RoundControlsLayout& layout)
{
    if (clicked(layout.pause) || pressed_any({Key::Escape, Key::P, Key::Space})) {
        return RoundAction::pause;
    }
    if (clicked(layout.slower) || pressed_any({Key::Minus, Key::KeypadSubtract})) {
        return RoundAction::slower;
    }
    // "+" shares its key with "=" on most layouts
    if (clicked(layout.faster) || pressed_any({Key::Equal, Key::KeypadAdd, Key::RightBracket})) {
        return RoundAction::faster;
    }
    return RoundAction::none;
}

void round_controls_show(const RoundControlsLayout& layout, const float speed, const float minSpeed, const float maxSpeed)
{
    with([&] {
        surface(layout.panel, withAlpha(panelColor, 225), withOpacity(borderColor, 0.3f), 12.0f);

        // pause: two bars
        icon_button(layout.pause, true);
        const float2 p = center(layout.pause);
        line(p.x - 4.0f, p.y - 7.0f, p.x - 4.0f, p.y + 7.0f);
        line(p.x + 4.0f, p.y - 7.0f, p.x + 4.0f, p.y + 7.0f);

        const bool canSlow = speed > minSpeed + 0.001f;
        icon_button(layout.slower, canSlow);
        const float2 s = center(layout.slower);
        line(s.x - 6.0f, s.y, s.x + 6.0f, s.y);

        const bool canSpeedUp = speed < maxSpeed - 0.001f;
        icon_button(layout.faster, canSpeedUp);
        const float2 f = center(layout.faster);
        line(f.x - 6.0f, f.y, f.x + 6.0f, f.y);
        line(f.x, f.y - 6.0f, f.x, f.y + 6.0f);

        noStroke();
        font_bold();
        textSize(16.0f);
        textAlign(TextAlignment::center);
        fill(speed > 1.0f + 0.001f ? accentColor : textColor);
        text(std::format("{:.2f}x", speed), layout.speed.left + layout.speed.width * 0.5f, layout.speed.top + layout.speed.height * 0.5f);
    });
}

PauseMenuLayout pause_menu_layout(const float width, const float height)
{
    constexpr float panelWidth = 300.0f;
    constexpr float panelHeight = 210.0f;
    constexpr float padding = 24.0f;
    constexpr float buttonHeight = 44.0f;

    const rect2f panel {(width - panelWidth) * 0.5f, (height - panelHeight) * 0.5f, panelWidth, panelHeight};

    rect2f content = inset(panel, padding, 0.0f);
    cutTop(content, 64.0f);
    const rect2f resume = cutTop(content, buttonHeight);
    cutTop(content, 12.0f);
    const rect2f quit = cutTop(content, buttonHeight);

    return PauseMenuLayout {.panel = panel, .resume = resume, .quit = quit};
}

RoundAction pause_menu_update(const PauseMenuLayout& layout)
{
    if (clicked(layout.resume) || pressed_any({Key::Escape, Key::P, Key::Space})) {
        return RoundAction::resume;
    }
    if (clicked(layout.quit)) {
        return RoundAction::quitRound;
    }
    return RoundAction::none;
}

void pause_menu_show(const PauseMenuLayout& layout, const float width, const float height)
{
    with([&] {
        noStroke();
        fill(rgba(0, 0, 0, 170));
        rect(0.0f, 0.0f, width, height);

        glow(layout.panel, accentColor, 0.7f, 16.0f, 22.0f);
        surface(layout.panel, panelColor, withOpacity(borderColor, 0.45f), 16.0f);

        font_medium();
        textSize(11.0f);
        textLetterSpacing(2.0f);
        textAlign(TextAlignment::center);
        fill(mutedTextColor);
        text("PAUSE", center(layout.panel).x, layout.panel.top + 34.0f);
        textLetterSpacing(0.0f);

        button(layout.resume, "Weiter", accentColor);
        button(layout.quit, "Runde beenden", dangerColor);

        font_medium();
        textSize(12.0f);
        fill(mutedTextColor);
        text("Coins dieser Runde bleiben erhalten", center(layout.panel).x, layout.quit.top + layout.quit.height + 18.0f);
    });
}
