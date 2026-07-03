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

    struct CornerRadius
    {
        float x;
        float y;

        static constexpr CornerRadius circular(float radius);
        static constexpr CornerRadius elliptical(float radiusX, float radiusY);
    };

    struct BorderRadius
    {
        CornerRadius topLeft;
        CornerRadius topRight;
        CornerRadius bottomRight;
        CornerRadius bottomLeft;

        static constexpr BorderRadius all(const CornerRadius& radius);
        static constexpr BorderRadius symmetric(float horizontal, float vertical);
        static constexpr BorderRadius circular(float radius);
        static constexpr BorderRadius elliptical(float radiusX, float radiusY);
    };
} // namespace p5cpp

namespace p5cpp
{
    inline constexpr CornerRadius CornerRadius::circular(float radius)
    {
        return CornerRadius {radius, radius};
    }

    inline constexpr CornerRadius CornerRadius::elliptical(float radiusX, float radiusY)
    {
        return CornerRadius {radiusX, radiusY};
    }
} // namespace p5cpp

namespace p5cpp
{
    inline constexpr BorderRadius BorderRadius::all(const CornerRadius& radius)
    {
        return BorderRadius {
            .topLeft = radius,
            .topRight = radius,
            .bottomRight = radius,
            .bottomLeft = radius,
        };
    }

    inline constexpr BorderRadius BorderRadius::symmetric(float horizontal, float vertical)
    {
        const CornerRadius cornerRadius = CornerRadius::elliptical(horizontal, vertical);

        return BorderRadius {
            .topLeft = cornerRadius,
            .topRight = cornerRadius,
            .bottomRight = cornerRadius,
            .bottomLeft = cornerRadius,
        };
    }

    inline constexpr BorderRadius BorderRadius::circular(float radius)
    {
        return BorderRadius::all(CornerRadius::circular(radius));
    }

    inline constexpr BorderRadius BorderRadius::elliptical(float radiusX, float radiusY)
    {
        return BorderRadius::all(CornerRadius::elliptical(radiusX, radiusY));
    }
} // namespace p5cpp
