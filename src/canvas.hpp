#pragma once

#include <p5.hpp>

#include "render_state_stack.hpp"
#include "draw_command_list.hpp"

namespace p5
{
    struct Canvas
    {
        std::shared_ptr<Framebuffer> framebuffer;
        RenderStateStack renderStates;
        DrawCommandList drawCommands;
    };
} // namespace p5
