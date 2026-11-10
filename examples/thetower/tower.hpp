#pragma once

#include <p5cpp/p5cpp.hpp>

#include "meters.hpp"

struct Tower
{
    p5::float2 position;
};

void tower_show(const Tower& tower, const Meters& range);
