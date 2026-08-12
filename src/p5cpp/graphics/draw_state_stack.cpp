#include <p5cpp/graphics/draw_state_stack.hpp>

namespace p5
{
    DrawStateStack::DrawStateStack()
        : m_stack(std::make_unique<DrawState[]>(16)),
          m_capacity(16),
          m_index(0)
    {
    }

    void DrawStateStack::push(const DrawState& state)
    {
        // Copy first: `state` may alias an element of m_stack (e.g. via peek()),
        // which would otherwise dangle once the buffer below is reallocated.
        const DrawState value = state;

        if (m_index + 1 >= m_capacity) {
            m_capacity *= 2;
            auto newStack = std::make_unique<DrawState[]>(m_capacity);
            std::copy(m_stack.get(), m_stack.get() + m_index + 1, newStack.get());
            m_stack = std::move(newStack);
        }

        m_stack[m_index + 1] = value;
        ++m_index;
    }

    void DrawStateStack::pop()
    {
        if (m_index == 0) {
            throw std::runtime_error("DrawStateStack::pop() called with no matching push()");
        }

        --m_index;
    }

    DrawState& DrawStateStack::peek()
    {
        return m_stack[m_index];
    }

    const DrawState& DrawStateStack::peek() const
    {
        return m_stack[m_index];
    }
} // namespace p5
