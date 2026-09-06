#include <p5cpp/graphics/matrix_stack.hpp>

namespace p5
{
    MatrixStack::MatrixStack()
        : m_stack()
    {
        m_stack.set(identityMatrix());
    }

    void MatrixStack::push(const matrix4x4& matrix)
    {
        m_stack.push(matrix);
    }

    void MatrixStack::pop()
    {
        m_stack.pop();
    }

    void MatrixStack::set(const matrix4x4& matrix)
    {
        m_stack.set(matrix);
    }

    matrix4x4& MatrixStack::peek()
    {
        return m_stack.peek();
    }

    const matrix4x4& MatrixStack::peek() const
    {
        return m_stack.peek();
    }
} // namespace p5
