#include <p5cpp/graphics/draw_state_stack.hpp>
#include <p5cpp/p5cpp.hpp>

namespace p5
{
    DrawStateStack::DrawStateStack()
        : m_stack(1)
    {
    }

    void DrawStateStack::push(const DrawState& state)
    {
        m_stack.push_back(state);
    }

    void DrawStateStack::pop()
    {
        if (m_stack.size() == 1) {
            error("DrawStateStack::pop() called with no matching push()");
            return;
        }

        m_stack.pop_back();
    }

    DrawState& DrawStateStack::peek()
    {
        return m_stack.back();
    }

    const DrawState& DrawStateStack::peek() const
    {
        return m_stack.back();
    }
} // namespace p5
