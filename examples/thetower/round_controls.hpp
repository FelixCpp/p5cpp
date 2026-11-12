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
    p5::rect2f panel;
    p5::rect2f pause;
    p5::rect2f slower;
    p5::rect2f speed;
    p5::rect2f faster;
};

RoundControlsLayout round_controls_layout(const p5::float2& topLeft);
RoundAction round_controls_update(const RoundControlsLayout& layout);
void round_controls_show(const RoundControlsLayout& layout, float speed, float minSpeed, float maxSpeed);

struct PauseMenuLayout
{
    p5::rect2f panel;
    p5::rect2f resume;
    p5::rect2f quit;
};

PauseMenuLayout pause_menu_layout(float width, float height);
RoundAction pause_menu_update(const PauseMenuLayout& layout);
void pause_menu_show(const PauseMenuLayout& layout, float width, float height);
