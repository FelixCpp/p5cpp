#pragma once

#include <p5cpp/graphics/shaping.hpp>

namespace p5cpp
{
    struct PathPoints;
    struct DrawBufferWriter;
} // namespace p5cpp

namespace p5cpp
{
    void tesselate_quads(DrawBufferWriter& scope, const PathPoints& points);
    void tesselate_quad_strip(DrawBufferWriter& scope, const PathPoints& points);
    void tesselate_triangles(DrawBufferWriter& scope, const PathPoints& points);
    void tesselate_triangle_strip(DrawBufferWriter& scope, const PathPoints& points);
    void tesselate_triangle_fan(DrawBufferWriter& scope, const PathPoints& points);
    void tesselate_polygon(DrawBufferWriter& scope, const PathPoints& points);
} // namespace p5cpp

namespace p5cpp
{
    void stroke_quads(DrawBufferWriter& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, const ComputeCircleSegmentCount& computeCircleSegmentCount);
    void stroke_quad_strip(DrawBufferWriter& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, const ComputeCircleSegmentCount& computeCircleSegmentCount);
    void stroke_triangles(DrawBufferWriter& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, const ComputeCircleSegmentCount& computeCircleSegmentCount);
    void stroke_triangle_strip(DrawBufferWriter& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, const ComputeCircleSegmentCount& computeCircleSegmentCount);
    void stroke_triangle_fan(DrawBufferWriter& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, const ComputeCircleSegmentCount& computeCircleSegmentCount);
    void stroke_lines(DrawBufferWriter& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, float miterLimit, float roundJoinThreshold, const ComputeCircleSegmentCount& computeCircleSegmentCount);
    void stroke_line_strip(DrawBufferWriter& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, const ComputeCircleSegmentCount& computeCircleSegmentCount);
    void stroke_line_loop(DrawBufferWriter& scope, const PathPoints& points, float strokeWeight, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, const ComputeCircleSegmentCount& computeCircleSegmentCount);
    void stroke_polygon(DrawBufferWriter& scope, const PathPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, bool close, const ComputeCircleSegmentCount& computeCircleSegmentCount);
} // namespace p5cpp
