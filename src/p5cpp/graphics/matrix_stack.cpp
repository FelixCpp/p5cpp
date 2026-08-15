#include <p5cpp/graphics/matrix_stack.hpp>

namespace p5
{
    MatrixStack::MatrixStack()
        : m_stack(std::make_unique<matrix4x4[]>(16)),
          m_capacity(16),
          m_index(0)
    {
        m_stack[0] = identityMatrix();
    }

    void MatrixStack::push(const matrix4x4& matrix)
    {
        const matrix4x4 value = matrix;

        if (m_index + 1 >= m_capacity) {
            m_capacity *= 2;
            auto newStack = std::make_unique<matrix4x4[]>(m_capacity);
            std::copy(m_stack.get(), m_stack.get() + m_index + 1, newStack.get());
            m_stack = std::move(newStack);
        }

        m_stack[m_index + 1] = value;
        ++m_index;
    }

    void MatrixStack::pop()
    {
        if (m_index == 0) {
            error("MatrixStack::pop() called with no matching push()");
            return;
        }

        --m_index;
    }

    void MatrixStack::set(const matrix4x4& matrix)
    {
        m_stack[m_index] = matrix;
    }

    matrix4x4& MatrixStack::peek()
    {
        return m_stack[m_index];
    }

    const matrix4x4& MatrixStack::peek() const
    {
        return m_stack[m_index];
    }
} // namespace p5
