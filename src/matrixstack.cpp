#include "matrixstack.hpp"

namespace p5
{
    MatrixStack::MatrixStack()
        : m_stack({matrix4x4::identity})
    {
    }

    void MatrixStack::push()
    {
        m_stack.push(m_stack.top());
    }

    void MatrixStack::pop()
    {
        if (m_stack.size() > 1) {
            m_stack.pop();
        }
    }

    matrix4x4& MatrixStack::peek()
    {
        return m_stack.top();
    }
} // namespace p5
