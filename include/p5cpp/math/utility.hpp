#pragma once

namespace p5cpp
{
    constexpr float remap(float value, float fromLow, float fromHigh, float toLow, float toHigh);
} // namespace p5cpp

namespace p5cpp
{
    inline constexpr float remap(float value, float fromLow, float fromHigh, float toLow, float toHigh)
    {
        return toLow + (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow);
    }
} // namespace p5cpp
