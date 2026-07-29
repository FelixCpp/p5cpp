#include <p5cpp/math/matrix4x4.hpp>

#include <cmath>

namespace p5cpp
{
    matrix4x4 translate(const matrix4x4& m, float x, float y)
    {
        return m * matrix4x4::translation(x, y);
    }

    matrix4x4 scale(const matrix4x4& m, float x, float y)
    {
        return m * matrix4x4::scaling(x, y);
    }

    matrix4x4 rotate(const matrix4x4& m, float radians)
    {
        return m * matrix4x4::rotation(radians);
    }

    matrix4x4& operator*=(matrix4x4& a, const matrix4x4& b)
    {
        return a = a * b;
    }

    matrix4x4 matrix4x4::rotation(float radians)
    {
        const float c = std::cos(radians);
        const float s = std::sin(radians);

        // clang-format off
        return {
            c,   s,   0.0f, 0.0f,
           -s,   c,   0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        // clang-format on
    }

    matrix4x4 matrix4x4::perspective(float fovY, float aspect, float near, float far)
    {
        const float f = 1.0f / std::tan(fovY * 0.5f);
        const float rcpDepth = 1.0f / (far - near);

        // clang-format off
        return {
            f / aspect, 0.0f, 0.0f, 0.0f,
            0.0f, f, 0.0f, 0.0f,
            0.0f, 0.0f, -(far + near) * rcpDepth, -1.0f,
            0.0f, 0.0f, -2.0f * far * near * rcpDepth, 0.0f
        };
        // clang-format on
    }

    matrix4x4 matrix4x4::lookAt(float2 eye, float2 center, float2 up)
    {
        const float2 f = normalized(center - eye);
        const float2 s = normalized(perpendicular(f));
        const float2 u = perpendicular(s);

        // clang-format off
        return {
            s.x, u.x, -f.x, 0.0f,
            s.y, u.y, -f.y, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            -dot(s, eye), -dot(u, eye), dot(f, eye), 1.0f
        };
        // clang-format on
    }
} // namespace p5cpp
