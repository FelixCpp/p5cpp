#pragma once

#include <p5cpp/math/matrix4x4.hpp>

#include <stack>

namespace p5cpp
{
    class MatrixStack
    {
    public:
        MatrixStack();

        void push(const matrix4x4& matrix);
        void pop();
        void reset();

        matrix4x4& peek();

    private:
        std::stack<matrix4x4> metrics;
    };
} // namespace p5cpp
