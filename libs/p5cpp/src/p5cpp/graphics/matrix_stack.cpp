#include <p5cpp/graphics/matrix_stack.hpp>

namespace p5
{
    MatrixStack::MatrixStack()
        : m_stack(1, identityMatrix())
    {
    }

    void MatrixStack::push(const matrix4x4& matrix)
    {
        m_stack.push_back(matrix);
    }

    void MatrixStack::pop()
    {
        if (m_stack.size() == 1) {
            error("MatrixStack::pop() called with no matching push()");
            return;
        }

        m_stack.pop_back();
    }

    void MatrixStack::set(const matrix4x4& matrix)
    {
        m_stack.back() = matrix;
    }

    matrix4x4& MatrixStack::peek()
    {
        return m_stack.back();
    }

    const matrix4x4& MatrixStack::peek() const
    {
        return m_stack.back();
    }
} // namespace p5
