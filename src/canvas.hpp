#pragma once

#include <p5.hpp>

#include "render_state_stack.hpp"

namespace p5
{
    struct Canvas
    {
        RenderStateStack renderStates;
    };
} // namespace p5
