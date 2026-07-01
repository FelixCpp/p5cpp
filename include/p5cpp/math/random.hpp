#pragma once

#include <cstdint>

namespace p5cpp
{
    float randomFloat(float min, float max);
    float randomFloat(float max);

    int randomInt(int min, int max);
    int randomInt(int max);

    void randomSeed(uint64_t seed);
} // namespace p5cpp
