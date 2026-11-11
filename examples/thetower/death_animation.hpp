#pragma once

#include <variant>

#include <p5cpp/p5cpp.hpp>

struct ParticleDeathAnimation
{
    p5::color_t color;
};

using DeathAnimation = std::variant<ParticleDeathAnimation>;
