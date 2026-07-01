#pragma once

#include <p5cpp/math/constants.hpp>

namespace p5cpp
{
    constexpr float radians(float degrees);
    constexpr float degrees(float radians);
} // namespace p5cpp

namespace p5cpp
{
    inline static constexpr float DEG_TO_RAD = PI / 180.0f;
    inline static constexpr float RAD_TO_DEG = 180.0f / PI;

    inline constexpr float radians(float degrees)
    {
        return degrees * DEG_TO_RAD;
    }

    inline constexpr float degrees(float radians)
    {
        return radians * RAD_TO_DEG;
    }
} // namespace p5cpp
