#pragma once

#include <p5cpp/p5cpp.hpp>

#include <memory>

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
        std::unique_ptr<matrix4x4[]> m_stack;
        size_t m_capacity;
        size_t m_index;
    };
} // namespace p5
