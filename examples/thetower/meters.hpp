#pragma once

#include <compare>

struct Meters
{
    float value;

    float toPixels() const;
    constexpr auto operator<=>(const Meters& other) const = default;
};
