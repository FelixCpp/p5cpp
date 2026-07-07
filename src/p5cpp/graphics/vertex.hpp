#pragma once

#include <p5cpp/math/value2.hpp>
#include <p5cpp/math/value4.hpp>

namespace p5cpp
{
    struct Vertex
    {
        float2 position;
        float2 texcoord;
        float4 color;
        uint32_t textureSlot;
    };
} // namespace p5cpp
