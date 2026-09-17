#pragma once

#include <p5cpp/graphics/draw_state.hpp>

#include <vector>

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

        size_t size() const { return m_stack.size(); }

    private:
        std::vector<DrawState> m_stack;
    };
} // namespace p5
