#pragma once

#include <stack>

#include <p5cpp.hpp>

namespace p5cpp
{
    struct MatrixStack
    {
        std::stack<matrix4x4> metrics;
    };

    MatrixStack matrix_stack_create();
    void matrix_stack_push(MatrixStack& stack, const matrix4x4& matrix);
    void matrix_stack_pop(MatrixStack& stack);
    void matrix_stack_reset(MatrixStack& stack);
    void matrix_stack_apply(MatrixStack& stack, const matrix4x4& matrix);
    void matrix_stack_set(MatrixStack& stack, const matrix4x4& matrix);

    matrix4x4& matrix_stack_peek(MatrixStack& stack);
} // namespace p5cpp
