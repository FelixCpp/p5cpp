#pragma once

#include "render_state.hpp"

namespace p5
{
    struct RenderStateStack
    {
        std::vector<RenderState> renderStates;
        size_t activeRenderStateIndex;
    };

    RenderStateStack render_state_stack_create();
    void render_state_stack_push(RenderStateStack& stack, const RenderState& state);
    void render_state_stack_pop(RenderStateStack& stack);
    void render_state_stack_clear(RenderStateStack& stack);
    RenderState& render_state_stack_peek(RenderStateStack& stack);
} // namespace p5
