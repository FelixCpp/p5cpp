#pragma once

#include <p5cpp/math/value2.hpp>
#include <p5cpp/graphics/color.hpp>

#include <span>

namespace p5cpp
{
    enum class ShapeType {
        lines,
        lineStrip,
        lineLoop,
        triangles,
        triangleStrip,
        triangleFan,
        quads,
        quadStrip,
        polygon,
    };

    struct PathPoints
    {
        size_t size;
        std::span<const float2> positions;
        std::span<const float2> texcoords;
        std::span<const color_t> colors;
    };
} // namespace p5cpp
