#include "render_state_stack.hpp"

namespace p5
{
    RenderStateStack render_state_stack_create()
    {
        return RenderStateStack {
            .renderStates = {render_state_create()},
            .activeRenderStateIndex = 0
        };
    }

    void render_state_stack_push(RenderStateStack& stack, const RenderState& state)
    {
        if (stack.activeRenderStateIndex < stack.renderStates.size() - 1) {
            stack.activeRenderStateIndex++;
            stack.renderStates[stack.activeRenderStateIndex] = state;
        } else {
            stack.renderStates.push_back(state);
            stack.activeRenderStateIndex = stack.renderStates.size() - 1;
        }
    }

    void render_state_stack_pop(RenderStateStack& stack)
    {
        if (stack.activeRenderStateIndex > 0) {
            stack.activeRenderStateIndex--;
        }
    }

    void render_state_stack_clear(RenderStateStack& stack)
    {
        stack.renderStates.resize(1);
        stack.renderStates[0] = RenderState();
        stack.activeRenderStateIndex = 0;
    }

    RenderState& render_state_stack_peek(RenderStateStack& stack)
    {
        return stack.renderStates[stack.activeRenderStateIndex];
    }
} // namespace p5
