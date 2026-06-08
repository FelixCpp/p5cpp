#include "canvas_stack.hpp"

namespace p5
{
    void canvas_stack_push(CanvasStack& stack, const Canvas& canvas)
    {
        stack.push_back(canvas);
    }

    void canvas_stack_pop(CanvasStack& stack)
    {
        if (not stack.empty()) {
            stack.pop_back();
        }
    }

    void canvas_stack_clear(CanvasStack& stack)
    {
        stack.clear();
    }

    bool canvas_stack_is_empty(const CanvasStack& stack)
    {
        return stack.empty();
    }

    Canvas& canvas_stack_peek(CanvasStack& stack)
    {
        return stack.back();
    }
} // namespace p5
