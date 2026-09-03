#include <p5cpp/graphics/graphics_stack.hpp>

namespace p5
{
    GraphicsStack::GraphicsStack()
    {
    }

    void GraphicsStack::push(Graphics graphics)
    {
        m_graphicsStack.push_back(std::move(graphics));
    }

    void GraphicsStack::pop()
    {
        if (m_graphicsStack.empty()) {
            error("GraphicsStack::pop() called with no matching push()");
            return;
        }

        m_graphicsStack.pop_back();
    }

    Graphics GraphicsStack::peek() const
    {
        if (not m_graphicsStack.empty()) {
            return m_graphicsStack.back();
        }

        return Graphics {};
    }
} // namespace p5
