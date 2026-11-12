#include "round_controls.hpp"

#include <format>

using namespace p5;
using namespace ui;

namespace
{
    constexpr float buttonSize = 32.0f;
    constexpr float speedWidth = 60.0f;
    constexpr float inset = 4.0f;

    bool pressed_any(const std::initializer_list<Key> keys)
    {
        for (const Key key : keys) {
            if (isKeyPressed(key)) {
                return true;
            }
        }
        return false;
    }

    void icon_button(const Rect& rect, const bool enabled)
    {
        noStroke();
        fill(enabled && contains(rect, mouse()) ? buttonHoverColor : buttonColor);
        p5::rect(rect.x, rect.y, rect.w, rect.h, BorderRadius::all(6.0f));
        stroke(enabled ? textColor : mutedTextColor);
        strokeWeight(2.5f);
        strokeCap(StrokeCap::round);
    }
}

RoundControlsLayout round_controls_layout(const float2& topLeft)
{
    const float x = topLeft.x + inset;
    const float y = topLeft.y + inset;
    const float gap = 4.0f;

    RoundControlsLayout layout = {};
    layout.pause = Rect {x, y, buttonSize, buttonSize};
    layout.slower = Rect {layout.pause.x + buttonSize + gap * 3.0f, y, buttonSize, buttonSize};
    layout.speed = Rect {layout.slower.x + buttonSize, y, speedWidth, buttonSize};
    layout.faster = Rect {layout.speed.x + speedWidth, y, buttonSize, buttonSize};
    layout.panel = Rect {topLeft.x, topLeft.y, layout.faster.x + buttonSize + inset - topLeft.x, buttonSize + inset * 2.0f};
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
        noStroke();
        fill(rgba(panelColor.r, panelColor.g, panelColor.b, 220));
        rect(layout.panel.x, layout.panel.y, layout.panel.w, layout.panel.h, BorderRadius::all(10.0f));

        // pause: two bars
        icon_button(layout.pause, true);
        const float2 p = {layout.pause.x + layout.pause.w * 0.5f, layout.pause.y + layout.pause.h * 0.5f};
        line(p.x - 4.0f, p.y - 7.0f, p.x - 4.0f, p.y + 7.0f);
        line(p.x + 4.0f, p.y - 7.0f, p.x + 4.0f, p.y + 7.0f);

        const bool canSlow = speed > minSpeed + 0.001f;
        icon_button(layout.slower, canSlow);
        const float2 s = {layout.slower.x + layout.slower.w * 0.5f, layout.slower.y + layout.slower.h * 0.5f};
        line(s.x - 6.0f, s.y, s.x + 6.0f, s.y);

        const bool canSpeedUp = speed < maxSpeed - 0.001f;
        icon_button(layout.faster, canSpeedUp);
        const float2 f = {layout.faster.x + layout.faster.w * 0.5f, layout.faster.y + layout.faster.h * 0.5f};
        line(f.x - 6.0f, f.y, f.x + 6.0f, f.y);
        line(f.x, f.y - 6.0f, f.x, f.y + 6.0f);

        noStroke();
        textSize(16.0f);
        textAlign(TextAlignment::center);
        fill(speed > 1.0f + 0.001f ? accentColor : textColor);
        text(std::format("{:.2f}x", speed), layout.speed.x + layout.speed.w * 0.5f, layout.speed.y + layout.speed.h * 0.5f);
    });
}

PauseMenuLayout pause_menu_layout(const float width, const float height)
{
    constexpr float panelWidth = 300.0f;
    constexpr float panelHeight = 210.0f;
    constexpr float padding = 24.0f;
    constexpr float buttonHeight = 44.0f;

    const float x = (width - panelWidth) * 0.5f;
    const float y = (height - panelHeight) * 0.5f;

    return PauseMenuLayout {
        .panel = Rect {x, y, panelWidth, panelHeight},
        .resume = Rect {x + padding, y + 64.0f, panelWidth - padding * 2.0f, buttonHeight},
        .quit = Rect {x + padding, y + 64.0f + buttonHeight + 12.0f, panelWidth - padding * 2.0f, buttonHeight},
    };
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
        fill(rgba(0, 0, 0, 150));
        rect(0.0f, 0.0f, width, height);

        fill(panelColor);
        stroke(buttonHoverColor);
        strokeWeight(1.0f);
        rect(layout.panel.x, layout.panel.y, layout.panel.w, layout.panel.h, BorderRadius::all(14.0f));
        noStroke();

        textSize(24.0f);
        textAlign(TextAlignment::center);
        fill(textColor);
        text("Pause", layout.panel.x + layout.panel.w * 0.5f, layout.panel.y + 34.0f);

        button(layout.resume, "Weiter", accentColor);
        button(layout.quit, "Runde beenden", dangerColor);

        textSize(12.0f);
        fill(mutedTextColor);
        text("Coins dieser Runde bleiben erhalten", layout.panel.x + layout.panel.w * 0.5f, layout.quit.y + layout.quit.h + 16.0f);
    });
}
