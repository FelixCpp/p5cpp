#pragma once

#include <p5cpp.hpp>

namespace p5cpp
{
    struct Vertex
    {
        float2 position;
        float2 texcoord;
        float4 color;
        float texIndex;
    };

    enum class ColorStyle {
        none,
        fill,
        stroke,
    };

    struct StrokePattern
    {
        std::span<const float> segments;
        float offset;
    };
} // namespace p5cpp
