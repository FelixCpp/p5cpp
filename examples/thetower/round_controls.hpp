#pragma once

#include <p5cpp/p5cpp.hpp>

#include "ui.hpp"

// speed and pause controls in the top left corner of the playfield, plus the pause overlay

enum class RoundAction
{
    none,
    pause,
    resume,
    slower,
    faster,
    quitRound,
};

struct RoundControlsLayout
{
    ui::Rect panel;
    ui::Rect pause;
    ui::Rect slower;
    ui::Rect speed;
    ui::Rect faster;
};

RoundControlsLayout round_controls_layout(const p5::float2& topLeft);
RoundAction round_controls_update(const RoundControlsLayout& layout);
void round_controls_show(const RoundControlsLayout& layout, float speed, float minSpeed, float maxSpeed);

struct PauseMenuLayout
{
    ui::Rect panel;
    ui::Rect resume;
    ui::Rect quit;
};

PauseMenuLayout pause_menu_layout(float width, float height);
RoundAction pause_menu_update(const PauseMenuLayout& layout);
void pause_menu_show(const PauseMenuLayout& layout, float width, float height);
