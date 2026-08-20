#include <p5cpp/p5cpp.hpp>

#include <random>

namespace p5
{
    namespace
    {
        std::mt19937& rng()
        {
            // thread_local, not a plain static: mt19937::operator() mutates the engine's internal
            // state with no internal synchronization, so a shared instance would be a data race
            // across threads calling random()/randomSeed() concurrently. Matches perlin_noise.cpp's
            // existing thread_local noise state for the same reason.
            static thread_local std::mt19937 engine(std::random_device {}());
            return engine;
        }
    } // namespace

    void randomSeed(uint32_t seed)
    {
        rng().seed(seed);
    }

    float random()
    {
        std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
        return distribution(rng());
    }

    float random(float max)
    {
        return random() * max;
    }

    float random(float min, float max)
    {
        return min + random() * (max - min);
    }
} // namespace p5
