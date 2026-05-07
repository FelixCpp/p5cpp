#pragma once

#include "meshwriter.hpp"
#include "p5.hpp"
#include "vertex.hpp"
#include "linepath.hpp"

namespace p5
{
    void generateSolidStroke(MeshWriter& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinAngleThreshold, bool close);
    void generateDashedStroke(MeshWriter& scope, const PathPoints& points, const StrokePattern& strokePattern, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinAngleThreshold, bool close);
} // namespace p5
