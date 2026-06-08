#pragma once

#include "draw_scope.hpp"
#include "linepath.hpp"

namespace p5
{
    void generate_solid_stroke(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinAngleThreshold, bool close);
} // namespace p5
