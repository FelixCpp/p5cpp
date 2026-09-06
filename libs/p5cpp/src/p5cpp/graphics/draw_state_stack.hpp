#pragma once

#include <p5cpp/graphics/draw_state.hpp>
#include <p5cpp/graphics/growable_stack.hpp>

namespace p5
{
    class DrawStateStack
    {
    public:
        DrawStateStack();

        void push(const DrawState& state);
        void pop();
        DrawState& peek();
        const DrawState& peek() const;

    private:
        GrowableStack<DrawState> m_stack;
    };
} // namespace p5
