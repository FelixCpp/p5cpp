#include <p5cpp/graphics/render_state_stack.hpp>

namespace p5cpp
{
    RenderStateStack::RenderStateStack()
    {
        stack.push(render_state_create());
    }

    void RenderStateStack::push()
    {
        stack.push(stack.top());
    }

    void RenderStateStack::pop()
    {
        if (stack.size() > 1) {
            stack.pop();
        }
    }

    void RenderStateStack::reset()
    {
        while (not stack.empty()) {
            stack.pop();
        }

        stack.push(render_state_create());
    }

    RenderState& RenderStateStack::peek()
    {
        return stack.top();
    }

    const RenderState& RenderStateStack::peek() const
    {
        return stack.top();
    }
} // namespace p5cpp
