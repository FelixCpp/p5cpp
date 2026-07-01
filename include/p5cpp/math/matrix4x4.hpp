#pragma once

#include <p5cpp/math/value2.hpp>

#include <array>

namespace p5cpp
{
    class matrix4x4
    {
    public:
        constexpr matrix4x4(
            float m00, float m01, float m02, float m03,
            float m10, float m11, float m12, float m13,
            float m20, float m21, float m22, float m23,
            float m30, float m31, float m32, float m33
        );

        void translate(float x, float y);
        void scale(float x, float y);
        void rotate(float radians);

        constexpr const float* data() const;

        constexpr float2 transformPoint(float x, float y) const;

        constexpr matrix4x4 operator*(const matrix4x4& other) const;
        matrix4x4& operator*=(const matrix4x4& other);

        static constexpr matrix4x4 translation(float x, float y);
        static constexpr matrix4x4 scaling(float x, float y);
        static constexpr matrix4x4 ortho(float left, float top, float right, float bottom, float near, float far);

        static matrix4x4 rotation(float radians);
        static matrix4x4 perspective(float fovY, float aspect, float near, float far);
        static matrix4x4 lookAt(float2 eye, float2 center, float2 up);

        static const matrix4x4 identity;

    private:
        std::array<float, 16> m;
    };
} // namespace p5cpp

namespace p5cpp
{
    inline constexpr matrix4x4::matrix4x4(
        float m00, float m01, float m02, float m03,
        float m10, float m11, float m12, float m13,
        float m20, float m21, float m22, float m23,
        float m30, float m31, float m32, float m33
    )
        : m {
              // clang-format off
            m00, m01, m02, m03,
            m10, m11, m12, m13,
            m20, m21, m22, m23,
            m30, m31, m32, m33
              // clang-format on
          }
    {
    }

    inline constexpr const float* matrix4x4::data() const
    {
        return m.data();
    }

    inline constexpr float2 matrix4x4::transformPoint(float x, float y) const
    {
        return {
            m[0] * x + m[4] * y + m[12],
            m[1] * x + m[5] * y + m[13],
        };
    }

    inline constexpr matrix4x4 matrix4x4::operator*(const matrix4x4& other) const
    {
        matrix4x4 result = matrix4x4::identity;

        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                result.m[row * 4 + col] =
                    m[row * 4 + 0] * other.m[0 * 4 + col] +
                    m[row * 4 + 1] * other.m[1 * 4 + col] +
                    m[row * 4 + 2] * other.m[2 * 4 + col] +
                    m[row * 4 + 3] * other.m[3 * 4 + col];
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
