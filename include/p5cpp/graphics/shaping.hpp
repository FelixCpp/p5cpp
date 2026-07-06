#pragma once

#include <p5cpp/math/value2.hpp>
#include <p5cpp/graphics/color.hpp>

#include <span>

namespace p5cpp
{
    struct ComputeCircleSegmentCount
    {
        virtual ~ComputeCircleSegmentCount() = default;
        virtual size_t operator()(float radius, float angle) const = 0;
    };
} // namespace p5cpp

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
}

namespace p5cpp
{
    enum class ArcMode {
        open,
        chord,
        pie,
    };
}

namespace p5cpp
{
    struct PathPoints
    {
        size_t size;
        std::span<const float2> positions;
        std::span<const float2> texcoords;
        std::span<const color_t> colors;
    };
} // namespace p5cpp

namespace p5cpp
{
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
    enum class StrokeCapStyle {
        butt,
        square,
        round,
    };

    struct StrokeCap
    {
        StrokeCapStyle start;
        StrokeCapStyle end;

        static const StrokeCap butt;
        static const StrokeCap square;
        static const StrokeCap round;
    };
} // namespace p5cpp

namespace p5cpp
{
    enum class StrokeJoin {
        miter,
        bevel,
        round
    };
}

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

namespace p5cpp
{
    inline constexpr StrokeCap StrokeCap::butt = StrokeCap {.start = StrokeCapStyle::butt, .end = StrokeCapStyle::butt};
    inline constexpr StrokeCap StrokeCap::square = StrokeCap {.start = StrokeCapStyle::square, .end = StrokeCapStyle::square};
    inline constexpr StrokeCap StrokeCap::round = StrokeCap {.start = StrokeCapStyle::round, .end = StrokeCapStyle::round};
} // namespace p5cpp
