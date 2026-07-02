#include <p5cpp/graphics/matrix_stack.hpp>

namespace p5cpp
{
    MatrixStack::MatrixStack() : metrics({matrix4x4::identity})
    {
    }

    void MatrixStack::push(const matrix4x4& matrix)
    {
        metrics.push(matrix);
    }

    void MatrixStack::pop()
    {
        if (metrics.size() > 1) {
            metrics.pop();
        }
    }

    void MatrixStack::reset()
    {
        while (metrics.size() > 1) {
            metrics.pop();
        }

        metrics.top() = matrix4x4::identity;
    }

    matrix4x4& MatrixStack::peek()
    {
        return metrics.top();
    }
} // namespace p5cpp
