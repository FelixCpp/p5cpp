#pragma once

#include "p5.hpp"

namespace p5
{
    struct Vertex
    {
        float2 position;
        float2 texcoord;
        float4 color;
        float texIndex;
    };

    enum class FillStyle {
        none,
        fill,
        stroke,
    };

    struct StrokePattern
    {
        std::span<const float> segments;
        float offset;
    };
} // namespace p5
