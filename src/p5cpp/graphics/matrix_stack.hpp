#pragma once

#include <p5cpp/math/matrix4x4.hpp>

#include <stack>

namespace p5cpp
{
    class MatrixStack
    {
    public:
        MatrixStack();

        void push();
        void pop();
        void reset();

        matrix4x4& peek();
        const matrix4x4& peek() const;

    private:
        std::stack<matrix4x4> metrics;
    };
} // namespace p5cpp
