#include <p5cpp/p5cpp.hpp>

#include <stb_perlin.h>
#include <random>

namespace p5
{
    namespace
    {
        inline static thread_local int32_t s_perlinSeed = std::random_device {}();
        inline static thread_local int32_t s_perlinOctaves = 4;
        inline static thread_local float s_perlinFalloff = 0.5f;

        // Sums octaves of stb's seeded gradient noise (lacunarity fixed at 2, like p5.js'
        // noiseDetail()). stb_perlin_fbm_noise3() can't be reused here since it always
        // samples with seed 0, ignoring noiseSeed().
        float perlinFbm(float x, float y, float z)
        {
            float sum = 0.0f;
            float amplitude = 0.5f;

            for (int32_t i = 0; i < s_perlinOctaves; ++i) {
                sum += amplitude * stb_perlin_noise3_seed(x, y, z, 0, 0, 0, s_perlinSeed);
                amplitude *= s_perlinFalloff;
                x *= 2.0f;
                y *= 2.0f;
                z *= 2.0f;
            }

            return sum;
        }
    } // namespace

    void noiseSeed(int32_t seed)
    {
        s_perlinSeed = seed;
    }

    void noiseDetail(int32_t octaves, float falloff)
    {
        if (octaves > 0) {
            s_perlinOctaves = octaves;
        }
        if (falloff > 0.0f) {
            s_perlinFalloff = falloff;
        }
    }

    float noise(float x)
    {
        return perlinFbm(x, 0.0f, 0.0f) * 0.5f + 0.5f;
    }

    float noise(float x, float y)
    {
        return perlinFbm(x, y, 0.0f) * 0.5f + 0.5f;
    }

    float noise(float x, float y, float z)
    {
        return perlinFbm(x, y, z) * 0.5f + 0.5f;
    }
} // namespace p5
