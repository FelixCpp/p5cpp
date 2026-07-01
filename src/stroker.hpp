#pragma once

#include <p5cpp.hpp>

namespace p5cpp
{
    struct DrawScope;
    struct PathPoints;
} // namespace p5cpp

namespace p5cpp
{
    void generate_solid_stroke(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinAngleThreshold, bool close);
} // namespace p5cpp
