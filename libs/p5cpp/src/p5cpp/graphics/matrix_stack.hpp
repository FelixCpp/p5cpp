#pragma once

#include <p5cpp/p5cpp.hpp>

#include <vector>

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

        size_t size() const { return m_stack.size(); }

    private:
        std::vector<matrix4x4> m_stack;
    };
} // namespace p5
