#pragma once

#include <p5cpp/p5cpp.hpp>

#include <vector>

namespace p5
{
    struct GraphicsStack
    {
    public:
        GraphicsStack();

        void push(Graphics graphics);
        void pop();
        Graphics peek() const;

    private:
        std::vector<Graphics> m_graphicsStack;
    };
} // namespace p5
