#include "p5.hpp"

namespace p5
{
    // clang-format off
    const matrix4x4 matrix4x4::identity = {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
    };
    // clang-format on

    matrix4x4 combine(const matrix4x4& a, const matrix4x4& b)
    {
        const float* m1 = a.m;
        const float* m2 = b.m;

        return {
            m1[0] * m2[0] + m1[4] * m2[1] + m1[8] * m2[2] + m1[12] * m2[3],
            m1[1] * m2[0] + m1[5] * m2[1] + m1[9] * m2[2] + m1[13] * m2[3],
            m1[2] * m2[0] + m1[6] * m2[1] + m1[10] * m2[2] + m1[14] * m2[3],
            m1[3] * m2[0] + m1[7] * m2[1] + m1[11] * m2[2] + m1[15] * m2[3],

            m1[0] * m2[4] + m1[4] * m2[5] + m1[8] * m2[6] + m1[12] * m2[7],
            m1[1] * m2[4] + m1[5] * m2[5] + m1[9] * m2[6] + m1[13] * m2[7],
            m1[2] * m2[4] + m1[6] * m2[5] + m1[10] * m2[6] + m1[14] * m2[7],
            m1[3] * m2[4] + m1[7] * m2[5] + m1[11] * m2[6] + m1[15] * m2[7],

            m1[0] * m2[8] + m1[4] * m2[9] + m1[8] * m2[10] + m1[12] * m2[11],
            m1[1] * m2[8] + m1[5] * m2[9] + m1[9] * m2[10] + m1[13] * m2[11],
            m1[2] * m2[8] + m1[6] * m2[9] + m1[10] * m2[10] + m1[14] * m2[11],
            m1[3] * m2[8] + m1[7] * m2[9] + m1[11] * m2[10] + m1[15] * m2[11],

            m1[0] * m2[12] + m1[4] * m2[13] + m1[8] * m2[14] + m1[12] * m2[15],
            m1[1] * m2[12] + m1[5] * m2[13] + m1[9] * m2[14] + m1[13] * m2[15],
            m1[2] * m2[12] + m1[6] * m2[13] + m1[10] * m2[14] + m1[14] * m2[15],
            m1[3] * m2[12] + m1[7] * m2[13] + m1[11] * m2[14] + m1[15] * m2[15],
        };
    }

    matrix4x4 translation(float tx, float ty)
    {
        // clang-format off
        return {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            tx, ty, 0.0f, 1.0f
        };
        // clang-format on
    }

    matrix4x4 scaling(float sx, float sy)
    {
        // clang-format off
        return {
            sx, 0.0f, 0.0f, 0.0f,
            0.0f, sy, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        // clang-format on
    }

    matrix4x4 rotation(radians_t angle)
    {
        const float c = std::cos(angle);
        const float s = std::sin(angle);

        // clang-format off
        return {
            c, s, 0.0f, 0.0f,
            -s, c, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        // clang-format on
    }

    matrix4x4 ortho(float left, float right, float top, float bottom, float near, float far)
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

    matrix4x4 perspective(float fovY, float aspect, float near, float far)
    {
        const float f = 1.0f / std::tan(fovY * 0.5f);
        const float rcpDepth = 1.0f / (near - far);

        // clang-format off
        return {
            f / aspect, 0.0f, 0.0f, 0.0f,
            0.0f, f, 0.0f, 0.0f,
            0.0f, 0.0f, (far + near) * rcpDepth, -1.0f,
            0.0f, 0.0f, far * near * rcpDepth * 2.0f, 0.0f
        };
        // clang-format on
    }

    matrix4x4 lookAt(float2 eye, float2 center, float2 up)
    {
        const float2 f = normalized(center - eye);
        const float2 s = normalized(perp(f));
        const float2 u = perp(s);

        // clang-format off
        return {
            s.x, u.x, 0.0f, 0.0f,
            s.y, u.y, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            -dot(s, eye), -dot(u, eye), 0.0f, 1.0f
        };
        // clang-format on
    }

    float2 transformPoint(const matrix4x4& matrix, float2 point)
    {
        return {
            matrix.m[0] * point.x + matrix.m[4] * point.y + matrix.m[12],
            matrix.m[1] * point.x + matrix.m[5] * point.y + matrix.m[13],
        };
    }
} // namespace p5
