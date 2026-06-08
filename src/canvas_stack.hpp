#pragma once

#include "canvas.hpp"

namespace p5
{
    // Temporarily, we can use a simple vector to manage the canvas stack, but in the future we might want to
    // implement a more complex data structure to manage the canvas stack, such as a linked list or a tree,
    // to allow for more complex rendering techniques and optimizations.
    typedef std::vector<Canvas> CanvasStack;

    void canvas_stack_push(CanvasStack& stack, const Canvas& canvas);
    void canvas_stack_pop(CanvasStack& stack);
    void canvas_stack_clear(CanvasStack& stack);
    bool canvas_stack_is_empty(const CanvasStack& stack);
    Canvas& canvas_stack_peek(CanvasStack& stack);
} // namespace p5
