#pragma once

#include <p5cpp/graphics/render_state.hpp>

#include <stack>

namespace p5cpp
{
    class RenderStateStack
    {
    public:
        RenderStateStack();

        void push();
        void pop();
        void reset();

        RenderState& peek();
        const RenderState& peek() const;

    private:
        std::stack<RenderState> stack;
    };
} // namespace p5cpp
