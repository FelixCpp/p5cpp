#include <p5cpp/math/random.hpp>

#include <random>

namespace p5cpp
{
    class XShiro256Random
    {
    public:
        explicit XShiro256Random(uint64_t seed)
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
} // namespace p5cpp

namespace p5cpp
{
    inline static XShiro256Random randomGenerator {std::random_device {}()};
}

namespace p5cpp
{
    void randomSeed(uint64_t seed) { randomGenerator = XShiro256Random(seed); }
    float randomFloat(float max) { return randomGenerator.nextFloat(0.0f, max); }
    float randomFloat(float min, float max) { return randomGenerator.nextFloat(min, max); }
    int randomInt(int max) { return randomGenerator.nextInt(0, max); }
    int randomInt(int min, int max) { return randomGenerator.nextInt(min, max); }
} // namespace p5cpp
