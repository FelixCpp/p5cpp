#pragma once

#include "p5.hpp"
#include "renderer.hpp"
#include "vertex.hpp"

namespace p5
{
    void generateSolidStroke(DrawScope& scope, const DrawPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinAngleThreshold, bool close);
    void generateDashedStroke(DrawScope& scope, const DrawPoints& points, const StrokePattern& strokePattern, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinAngleThreshold, bool close);
} // namespace p5
