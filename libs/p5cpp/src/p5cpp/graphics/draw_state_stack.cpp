#include <p5cpp/graphics/draw_state_stack.hpp>

namespace p5
{
    DrawStateStack::DrawStateStack()
        : m_stack()
    {
    }

    void DrawStateStack::push(const DrawState& state)
    {
        m_stack.push(state);
    }

    void DrawStateStack::pop()
    {
        m_stack.pop();
    }

    DrawState& DrawStateStack::peek()
    {
        return m_stack.peek();
    }

    const DrawState& DrawStateStack::peek() const
    {
        return m_stack.peek();
    }
} // namespace p5
