#include <p5cpp/p5cpp.hpp>

#include <array>
#include <cmath>
#include <cstdint>

namespace p5
{
    namespace
    {
        constexpr int PERLIN_YWRAPB = 4;
        constexpr int PERLIN_YWRAP = 1 << PERLIN_YWRAPB;
        constexpr int PERLIN_ZWRAPB = 8;
        constexpr int PERLIN_ZWRAP = 1 << PERLIN_ZWRAPB;
        constexpr int PERLIN_SIZE = 4095;

        int perlinOctaves = 4;
        float perlinAmpFalloff = 0.5f;

        std::array<float, PERLIN_SIZE + 1> perlin;
        bool perlinInitialized = false;

        float scaledCosine(float i)
        {
            return 0.5f * (1.0f - std::cos(i * PI));
        }

        void ensurePerlinInitialized()
        {
            if (perlinInitialized)
                return;

            for (float& value : perlin) {
                value = random();
            }
            perlinInitialized = true;
        }

        struct Lcg
        {
            static constexpr uint32_t a = 1664525u;
            static constexpr uint32_t c = 1013904223u;

            uint32_t z = 0;

            float next()
            {
                z = a * z + c;
                return static_cast<float>(z) / 4294967296.0f;
            }
        };
    } // namespace

    void noiseSeed(uint32_t seed)
    {
        Lcg lcg {.z = seed};

        for (float& value : perlin) {
            value = lcg.next();
        }
        perlinInitialized = true;
    }

    void noiseDetail(int octaves, float falloff)
    {
        if (octaves > 0) {
            perlinOctaves = octaves;
        }
        if (falloff > 0.0f) {
            perlinAmpFalloff = falloff;
        }
    }

    float noise(float x, float y, float z)
    {
        ensurePerlinInitialized();

        if (x < 0.0f) x = -x;
        if (y < 0.0f) y = -y;
        if (z < 0.0f) z = -z;

        int xi = static_cast<int>(std::floor(x));
        int yi = static_cast<int>(std::floor(y));
        int zi = static_cast<int>(std::floor(z));
        float xf = x - static_cast<float>(xi);
        float yf = y - static_cast<float>(yi);
        float zf = z - static_cast<float>(zi);

        float r = 0.0f;
        float ampl = 0.5f;

        for (int o = 0; o < perlinOctaves; ++o) {
            int of = xi + (yi << PERLIN_YWRAPB) + (zi << PERLIN_ZWRAPB);

            const float rxf = scaledCosine(xf);
            const float ryf = scaledCosine(yf);

            float n1 = perlin[of & PERLIN_SIZE];
            n1 += rxf * (perlin[(of + 1) & PERLIN_SIZE] - n1);
            float n2 = perlin[(of + PERLIN_YWRAP) & PERLIN_SIZE];
            n2 += rxf * (perlin[(of + PERLIN_YWRAP + 1) & PERLIN_SIZE] - n2);
            n1 += ryf * (n2 - n1);

            of += PERLIN_ZWRAP;
            n2 = perlin[of & PERLIN_SIZE];
            n2 += rxf * (perlin[(of + 1) & PERLIN_SIZE] - n2);
            float n3 = perlin[(of + PERLIN_YWRAP) & PERLIN_SIZE];
            n3 += rxf * (perlin[(of + PERLIN_YWRAP + 1) & PERLIN_SIZE] - n3);
            n2 += ryf * (n3 - n2);

            n1 += scaledCosine(zf) * (n2 - n1);

            r += n1 * ampl;
            ampl *= perlinAmpFalloff;

            xi <<= 1;
            xf *= 2.0f;
            yi <<= 1;
            yf *= 2.0f;
            zi <<= 1;
            zf *= 2.0f;

            if (xf >= 1.0f) {
                xi++;
                xf -= 1.0f;
            }
            if (yf >= 1.0f) {
                yi++;
                yf -= 1.0f;
            }
            if (zf >= 1.0f) {
                zi++;
                zf -= 1.0f;
            }
        }

        return r;
    }

    float noise(float x, float y)
    {
        return noise(x, y, 0.0f);
    }

    float noise(float x)
    {
        return noise(x, 0.0f, 0.0f);
    }
} // namespace p5
