#include "p5.hpp"

#include <numbers>
#include <random>

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
        const float* m1 = a.m.data();
        const float* m2 = b.m.data();

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

    matrix4x4 rotation(float angle)
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

    matrix4x4 ortho(float left, float top, float right, float bottom, float near, float far)
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

namespace p5
{
    float radians(float degrees) { return degrees * (std::numbers::pi_v<float> / 180.0f); }
    float degrees(float radians) { return radians * (180.0f / std::numbers::pi_v<float>); }
    float remap(float value, float fromLow, float fromHigh, float toLow, float toHigh)
    {
        const float t = (value - fromLow) / (fromHigh - fromLow);
        return std::lerp(toLow, toHigh, t);
    }
} // namespace p5

namespace p5
{
    class XShiro256Random
    {
    public:
        explicit XShiro256Random(uint64_t seed = std::random_device {}())
        {
            // Seed mit SplitMix64 initialisieren – xoshiro braucht einen
            // guten initialen Zustand, ein einfacher Seed-Wert reicht nicht
            setSeed(seed);
        }

        void setSeed(uint64_t seed)
        {
            state[0] = splitMix64(seed);
            state[1] = splitMix64(state[0]);
            state[2] = splitMix64(state[1]);
            state[3] = splitMix64(state[2]);
        }

        // Rohes uint64 [0, 2^64)
        uint64_t next()
        {
            const uint64_t result = rotl(state[0] + state[3], 23) + state[0];

            const uint64_t t = state[1] << 17;
            state[2] ^= state[0];
            state[3] ^= state[1];
            state[1] ^= state[2];
            state[0] ^= state[3];
            state[2] ^= t;
            state[3] = rotl(state[3], 45);

            return result;
        }

        // Float [0.0f, 1.0f)
        float nextFloat()
        {
            // Obere 23 Bits als Mantisse nutzen
            return (next() >> 41) * (1.0f / (1ull << 23));
        }

        // Float [min, max)
        float nextFloat(float min, float max)
        {
            return min + nextFloat() * (max - min);
        }

        // Int [min, max]
        int32_t nextInt(int32_t min, int32_t max)
        {
            // Debiased Modulo – vermeidet den klassischen Modulo-Bias
            const uint64_t range = static_cast<uint64_t>(max - min) + 1;
            const uint64_t threshold = (UINT64_MAX - range + 1) % range;
            uint64_t r;
            do {
                r = next();
            } while (r < threshold);
            return static_cast<int32_t>(r % range) + min;
        }

    private:
        uint64_t state[4];

        static uint64_t rotl(uint64_t x, int k)
        {
            return (x << k) | (x >> (64 - k));
        }

        static uint64_t splitMix64(uint64_t x)
        {
            x += 0x9e3779b97f4a7c15ull;
            x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
            x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
            return x ^ (x >> 31);
        }
    };

    inline static XShiro256Random randomGenerator;

    void randomSeed(uint64_t seed) { randomGenerator = XShiro256Random(seed); }
    float random(float max) { return randomGenerator.nextFloat(0.0f, max); }
    float random(float min, float max) { return randomGenerator.nextFloat(min, max); }
} // namespace p5
