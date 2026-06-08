#include "matrix_stack.hpp"

namespace p5
{
    MatrixStack matrix_stack_create()
    {
        return MatrixStack {
            .metrics = std::stack<matrix4x4> {
                {matrix4x4::identity}
            },
        };
    }

    void matrix_stack_push(MatrixStack& stack, const matrix4x4& matrix)
    {
        stack.metrics.push(matrix);
    }

    void matrix_stack_pop(MatrixStack& stack)
    {
        if (stack.metrics.size() > 1) {
            stack.metrics.pop();
        }
    }

    void matrix_stack_reset(MatrixStack& stack)
    {
        while (stack.metrics.size() > 1) {
            stack.metrics.pop();
        }

        stack.metrics.top() = matrix4x4::identity;
    }

    void matrix_stack_apply(MatrixStack& stack, const matrix4x4& matrix)
    {
        stack.metrics.top() = combine(stack.metrics.top(), matrix);
    }

    void matrix_stack_set(MatrixStack& stack, const matrix4x4& matrix)
    {
        stack.metrics.top() = matrix;
    }

    matrix4x4& matrix_stack_peek(MatrixStack& stack)
    {
        return stack.metrics.top();
    }
} // namespace p5
