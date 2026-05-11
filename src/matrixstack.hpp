#pragma once

#include <stack>

#include "p5.hpp"

namespace p5
{
    class MatrixStack
    {
    public:
        MatrixStack();

        void push();
        void pop();

        matrix4x4& peek();

    private:
        std::stack<matrix4x4> m_stack;
    };
} // namespace p5
