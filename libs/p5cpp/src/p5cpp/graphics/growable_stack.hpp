#pragma once

#include <p5cpp/p5cpp.hpp>

#include <algorithm>
#include <memory>

namespace p5
{
    template <typename T>
    class GrowableStack
    {
    public:
        GrowableStack()
            : m_stack(std::make_unique<T[]>(16)),
              m_capacity(16),
              m_index(0)
        {
        }

        void push(const T& value)
        {
            const T copy = value;

            if (m_index + 1 >= m_capacity) {
                m_capacity *= 2;
                auto newStack = std::make_unique<T[]>(m_capacity);
                std::copy(m_stack.get(), m_stack.get() + m_index + 1, newStack.get());
                m_stack = std::move(newStack);
            }

            m_stack[m_index + 1] = copy;
            ++m_index;
        }

        void pop()
        {
            if (m_index == 0) {
                error("GrowableStack::pop() called with no matching push()");
                return;
            }

            --m_index;
        }

        T& peek() { return m_stack[m_index]; }
        const T& peek() const { return m_stack[m_index]; }

        void set(const T& value) { m_stack[m_index] = value; }

    private:
        std::unique_ptr<T[]> m_stack;
        size_t m_capacity;
        size_t m_index;
    };
} // namespace p5
