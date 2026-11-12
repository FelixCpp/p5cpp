#include "meters.hpp"

namespace
{
    constexpr float pixelsPerMeter = 10.0f;
}

float Meters::toPixels() const
{
    return value * pixelsPerMeter;
}
