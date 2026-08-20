#include <p5cpp/graphics/framebuffer_stack.hpp>

namespace p5
{
    FramebufferStack::FramebufferStack()
    {
    }

    void FramebufferStack::push(std::shared_ptr<Framebuffer> framebuffer)
    {
        m_framebufferStack.push_back(std::move(framebuffer));
    }

    void FramebufferStack::pop()
    {
        if (m_framebufferStack.empty()) {
            error("FramebufferStack::pop() called with no matching push()");
            return;
        }

        m_framebufferStack.pop_back();
    }

    std::shared_ptr<Framebuffer> FramebufferStack::peek() const
    {
        if (not m_framebufferStack.empty()) {
            return m_framebufferStack.back();
        }

        return nullptr;
    }
} // namespace p5
