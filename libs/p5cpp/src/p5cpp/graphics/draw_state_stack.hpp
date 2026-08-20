#pragma once

#include <p5cpp/graphics/draw_state.hpp>

#include <memory>

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
        std::unique_ptr<DrawState[]> m_stack;
        size_t m_capacity;
        size_t m_index;
    };
} // namespace p5
