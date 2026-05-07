#pragma once

#include "drawscope.hpp"
#include "p5.hpp"
#include "vertex.hpp"
#include "linepath.hpp"

namespace p5
{
    DrawScopeResult generateSolidStroke(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinAngleThreshold, bool close);
    DrawScopeResult generateDashedStroke(DrawScope& scope, const PathPoints& points, const StrokePattern& strokePattern, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinAngleThreshold, bool close);
} // namespace p5
