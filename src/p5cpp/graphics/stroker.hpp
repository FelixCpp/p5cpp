#pragma once

#include <p5cpp/graphics/shaping.hpp>

namespace p5cpp
{
    struct DrawBufferWriter;
} // namespace p5cpp

namespace p5cpp
{
    void generate_solid_stroke(
        DrawBufferWriter& writer,
        const PathPoints& points,
        float strokeWeight,
        StrokeCap strokeCap,
        StrokeJoin strokeJoin,
        float miterLimit,
        float roundJoinAngleThreshold,
        bool close,
        const ComputeCircleSegmentCount& computeCircleSegmentCount
    );
} // namespace p5cpp
