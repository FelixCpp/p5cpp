#pragma once

#include "linepath.hpp"
#include "drawscope.hpp"

namespace p5
{
    void tesselate_quads(DrawScope& scope, const PathPoints& points);
    void tesselate_quad_strip(DrawScope& scope, const PathPoints& points);
    void tesselate_triangles(DrawScope& scope, const PathPoints& points);
    void tesselate_triangle_strip(DrawScope& scope, const PathPoints& points);
    void tesselate_triangle_fan(DrawScope& scope, const PathPoints& points);
    void tesselate_lines(DrawScope& scope, const PathPoints& points);
    void tesselate_line_strip(DrawScope& scope, const PathPoints& points);
    void tesselate_line_loop(DrawScope& scope, const PathPoints& points);
    void tesselate_polygon(DrawScope& scope, const PathPoints& points);
} // namespace p5

namespace p5
{
    void stroke_quads(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold);
    void stroke_quad_strip(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold);
    void stroke_triangles(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold);
    void stroke_triangle_strip(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold);
    void stroke_triangle_fan(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold);
    void stroke_lines(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, float miterLimit, float roundJoinThreshold);
    void stroke_line_strip(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold);
    void stroke_line_loop(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold);
    void stroke_polygon(DrawScope& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold);
} // namespace p5
