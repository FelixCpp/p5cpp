#pragma once

#include <p5cpp/p5cpp.hpp>

#include <vector>

namespace p5
{
    struct FramebufferStack
    {
    public:
        FramebufferStack();

        void push(std::shared_ptr<Framebuffer> framebuffer);
        void pop();
        std::shared_ptr<Framebuffer> peek() const;

    private:
        std::vector<std::shared_ptr<Framebuffer>> m_framebufferStack;
    };
} // namespace p5
