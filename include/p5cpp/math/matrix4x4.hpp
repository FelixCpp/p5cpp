#pragma once

#include <p5cpp/math/value2.hpp>

#include <array>

namespace p5cpp
{
    struct matrix4x4
    {
        constexpr matrix4x4(
            float m00, float m01, float m02, float m03,
            float m10, float m11, float m12, float m13,
            float m20, float m21, float m22, float m23,
            float m30, float m31, float m32, float m33
        );

        static constexpr matrix4x4 translation(float x, float y);
        static constexpr matrix4x4 scaling(float x, float y);
        static constexpr matrix4x4 ortho(float left, float top, float right, float bottom, float near, float far);

        static matrix4x4 rotation(float radians);
        static matrix4x4 perspective(float fovY, float aspect, float near, float far);
        static matrix4x4 lookAt(float2 eye, float2 center, float2 up);

        static const matrix4x4 identity;

        std::array<float, 16> elements;
    };

    constexpr float2 transformPoint(const matrix4x4& m, float x, float y);

    matrix4x4 translate(const matrix4x4& m, float x, float y);
    matrix4x4 scale(const matrix4x4& m, float x, float y);
    matrix4x4 rotate(const matrix4x4& m, float radians);

    constexpr matrix4x4 operator*(const matrix4x4& a, const matrix4x4& b);
    matrix4x4& operator*=(matrix4x4& a, const matrix4x4& b);
} // namespace p5cpp

namespace p5cpp
{
    inline constexpr matrix4x4::matrix4x4(
        float m00, float m01, float m02, float m03,
        float m10, float m11, float m12, float m13,
        float m20, float m21, float m22, float m23,
        float m30, float m31, float m32, float m33
    )
        : elements {
              // clang-format off
            m00, m01, m02, m03,
            m10, m11, m12, m13,
            m20, m21, m22, m23,
            m30, m31, m32, m33
              // clang-format on
          }
    {
    }

    inline constexpr float2 transformPoint(const matrix4x4& m, float x, float y)
    {
        return {
            m.elements[0] * x + m.elements[4] * y + m.elements[12],
            m.elements[1] * x + m.elements[5] * y + m.elements[13],
        };
    }

    inline constexpr matrix4x4 operator*(const matrix4x4& a, const matrix4x4& b)
    {
        matrix4x4 result = matrix4x4::identity;

        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                result.elements[row * 4 + col] =
                    a.elements[row * 4 + 0] * b.elements[0 * 4 + col] +
                    a.elements[row * 4 + 1] * b.elements[1 * 4 + col] +
                    a.elements[row * 4 + 2] * b.elements[2 * 4 + col] +
                    a.elements[row * 4 + 3] * b.elements[3 * 4 + col];
            }
        }

        return result;
    }

    inline constexpr matrix4x4 matrix4x4::translation(float x, float y)
    {
        // clang-format off
        return {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            x,    y,    0.0f, 1.0f,
        };
        // clang-format on
    }

    inline constexpr matrix4x4 matrix4x4::scaling(float x, float y)
    {
        // clang-format off
        return {
            x,    0.0f, 0.0f, 0.0f,
            0.0f, y,    0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        // clang-format on
    }

    inline constexpr matrix4x4 matrix4x4::ortho(float left, float top, float right, float bottom, float near, float far)
    {
        const float rcpWidth = 1.0f / (right - left);
        const float rcpHeight = 1.0f / (bottom - top);
        const float rcpDepth = 1.0f / (far - near);
        const float tx = -(right + left) * rcpWidth;
        const float ty = -(bottom + top) * rcpHeight;

        // clang-format off
        return {
            2.0f * rcpWidth, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f * rcpHeight, 0.0f, 0.0f,
            0.0f, 0.0f, -rcpDepth, 0.0f,
            tx, ty, -near * rcpDepth, 1.0f
        };
        // clang-format on
    }

    // clang-format off
    inline constexpr matrix4x4 matrix4x4::identity = matrix4x4 {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    // clang-format on

} // namespace p5cpp
