#include <p5cpp/math/matrix4x4.hpp>

#include <cmath>

namespace p5cpp
{
    void matrix4x4::translate(float x, float y)
    {
        *this *= translation(x, y);
    }

    void matrix4x4::scale(float x, float y)
    {
        *this *= scaling(x, y);
    }

    void matrix4x4::rotate(float radians)
    {
        *this *= rotation(radians);
    }

    matrix4x4& matrix4x4::operator*=(const matrix4x4& rhs)
    {
        return *this = *this * rhs;
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
        const float2 f = (center - eye).normalized();
        const float2 s = f.perpendicular().normalized();
        const float2 u = s.perpendicular();

        // clang-format off
        return {
            s.x, u.x, -f.x, 0.0f,
            s.y, u.y, -f.y, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            -s.dot(eye), -u.dot(eye), f.dot(eye), 1.0f
        };
        // clang-format on
    }
} // namespace p5cpp
