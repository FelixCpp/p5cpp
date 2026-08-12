#include <p5cpp/p5cpp.hpp>

#include <random>

namespace p5
{
    namespace
    {
        std::mt19937& rng()
        {
            static std::mt19937 engine(std::random_device {}());
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
