#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/growable_stack.hpp>

namespace p5
{
    class MatrixStack
    {
    public:
        MatrixStack();

        void push(const matrix4x4& matrix);
        void pop();
        matrix4x4& peek();

        void set(const matrix4x4& matrix);

        const matrix4x4& peek() const;

    private:
        GrowableStack<matrix4x4> m_stack;
    };
} // namespace p5
