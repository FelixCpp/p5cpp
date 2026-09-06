#include <p5cpp/graphics/shape_builder.hpp>

#include <cmath>
#include <algorithm>

namespace p5
{
    int curveSegmentCount(float controlPolygonLength)
    {
        // A non-finite length (e.g. a NaN/Inf control point sneaking in from bezierVertex()/
        // quadraticVertex()/curveVertex()'s unvalidated float arguments) makes std::ceil() return
        // NaN/Inf too, and casting that to int is undefined behavior -- fall back to the minimum
        // segment count instead of casting a non-finite value.
        if (not std::isfinite(controlPolygonLength))
            return 8;
        return std::clamp(static_cast<int>(std::ceil(controlPolygonLength / 3.0f)), 8, 128);
    }
} // namespace p5

namespace p5
{
    ShapeBuilder::ShapeBuilder()
        : m_isBuilding(false),
          m_mode(ShapeMode::points)
    {
    }

    void ShapeBuilder::beginShape(ShapeMode mode)
    {
        if (m_isBuilding) {
            error("ShapeBuilder::beginShape() called while already building a shape");
            return;
        }

        m_isBuilding = true;
        m_mode = mode;
        m_positions.clear();
        m_texCoords.clear();
        m_fillColors.clear();
        m_strokeColors.clear();
        m_curvePoints.clear();
    }

    BuiltShape ShapeBuilder::endShape()
    {
        if (not m_isBuilding) {
            error("ShapeBuilder::endShape() called while not building a shape");

            return BuiltShape {
                .mode = {},
                .vertexCount = 0,
                .positions = {},
                .texCoords = {},
                .fillColors = {},
                .strokeColors = {}
            };
        }

        m_isBuilding = false;

        return BuiltShape {
            .mode = m_mode,
            .vertexCount = m_positions.size(),
            .positions = m_positions,
            .texCoords = m_texCoords,
            .fillColors = m_fillColors,
            .strokeColors = m_strokeColors
        };
    }

    void ShapeBuilder::vertex(float x, float y, color_t fillColor, color_t strokeColor)
    {
        vertex(x, y, 0.0f, 0.0f, fillColor, strokeColor);
    }

    void ShapeBuilder::vertex(float x, float y, float u, float v, color_t fillColor, color_t strokeColor)
    {
        if (not m_isBuilding) {
            error("ShapeBuilder::vertex() called while not building a shape");
            return;
        }

        m_positions.push_back({x, y});
        m_texCoords.push_back({u, v});
        m_fillColors.push_back(fillColor);
        m_strokeColors.push_back(strokeColor);
    }

    void ShapeBuilder::bezierVertex(float controlX1, float controlY1, float controlX2, float controlY2, float endX, float endY)
    {
        if (not m_isBuilding) {
            error("ShapeBuilder::bezierVertex() called while not building a shape");
            return;
        }

        if (m_positions.empty()) {
            error("ShapeBuilder::bezierVertex() called with no starting vertex");
            return;
        }

        const float2 startPoint = m_positions.back();
        const float2 controlPoint1 = {controlX1, controlY1};
        const float2 controlPoint2 = {controlX2, controlY2};
        const float2 endPoint = {endX, endY};

        const float length = distance(startPoint, controlPoint1) + distance(controlPoint1, controlPoint2) + distance(controlPoint2, endPoint);
        const int segments = curveSegmentCount(length);
        for (int i = 1; i <= segments; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(segments);
            const float u = 1.0f - t;
            const float2 point = u * u * u * startPoint + 3.0f * u * u * t * controlPoint1 + 3.0f * u * t * t * controlPoint2 + t * t * t * endPoint;
            vertex(point.x, point.y, m_texCoords.back().x, m_texCoords.back().y, m_fillColors.back(), m_strokeColors.back());
        }
    }

    void ShapeBuilder::quadraticVertex(float controlX, float controlY, float endX, float endY)
    {
        if (not m_isBuilding) {
            error("ShapeBuilder::quadraticVertex() called while not building a shape");
            return;
        }

        if (m_positions.empty()) {
            error("ShapeBuilder::quadraticVertex() called with no starting vertex");
            return;
        }

        const float2 startPoint = m_positions.back();
        const float2 controlPoint = {controlX, controlY};
        const float2 endPoint = {endX, endY};

        const float length = distance(startPoint, controlPoint) + distance(controlPoint, endPoint);
        const int segments = curveSegmentCount(length);
        for (int i = 1; i <= segments; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(segments);
            const float u = 1.0f - t;
            const float2 point = u * u * startPoint + 2.0f * u * t * controlPoint + t * t * endPoint;
            vertex(point.x, point.y, m_texCoords.back().x, m_texCoords.back().y, m_fillColors.back(), m_strokeColors.back());
        }
    }

    void ShapeBuilder::curveVertex(float x, float y, float tightness, color_t fillColor, color_t strokeColor)
    {
        if (not m_isBuilding) {
            error("ShapeBuilder::curveVertex() called while not building a shape");
            return;
        }

        m_curvePoints.push_back({x, y});
        if (m_curvePoints.size() < 4) {
            return;
        }

        const size_t n = m_curvePoints.size();
        const float2 p0 = m_curvePoints[n - 4];
        const float2 p1 = m_curvePoints[n - 3];
        const float2 p2 = m_curvePoints[n - 2];
        const float2 p3 = m_curvePoints[n - 1];

        // `tightness` == 0 reproduces the standard (uniform) Catmull-Rom spline; increasing it towards 1
        // pulls the curve straight through p1/p2 as p5.js's curveTightness() does.
        const float tangentScale = (1.0f - tightness) / 6.0f;
        const float2 controlPoint1 = p1 + (p2 - p0) * tangentScale;
        const float2 controlPoint2 = p2 - (p3 - p1) * tangentScale;

        // curveVertex() is called without a preceding vertex() (per the p5.js pattern), so seed the
        // shape's first output vertex from the Catmull-Rom segment's own start point.
        if (m_positions.empty()) {
            vertex(p1.x, p1.y, fillColor, strokeColor);
        }

        const float length = distance(p1, controlPoint1) + distance(controlPoint1, controlPoint2) + distance(controlPoint2, p2);
        const int segments = curveSegmentCount(length);
        for (int i = 1; i <= segments; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(segments);
            const float u = 1.0f - t;
            const float2 point = u * u * u * p1 + 3.0f * u * u * t * controlPoint1 + 3.0f * u * t * t * controlPoint2 + t * t * t * p2;
            vertex(point.x, point.y, m_texCoords.back().x, m_texCoords.back().y, m_fillColors.back(), m_strokeColors.back());
        }
    }
} // namespace p5
