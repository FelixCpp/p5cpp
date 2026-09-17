#include <p5cpp/graphics/shape_builder.hpp>
#include <p5cpp/graphics/curve_tessellation.hpp>

#include <cmath>

namespace p5
{
    int curveSegmentCount(float controlPolygonLength)
    {
        if (not std::isfinite(controlPolygonLength))
            return 8;
        return segmentCountForArcLength(controlPolygonLength, 3.0f, 8, 128);
    }
} // namespace p5

namespace p5
{
    ShapeBuilder::ShapeBuilder()
        : m_isBuilding(false),
          m_mode(ShapeMode::points)
    {
    }

    bool ShapeBuilder::requireBuilding(const char* callerName) const
    {
        if (not m_isBuilding) {
            error("ShapeBuilder::{}() called while not building a shape", callerName);
            return false;
        }
        return true;
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
        m_curvePointCount = 0;
    }

    BuiltShape ShapeBuilder::endShape()
    {
        if (not requireBuilding("endShape")) {
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
        if (not requireBuilding("vertex")) {
            return;
        }

        m_positions.push_back({x, y});
        m_texCoords.push_back({u, v});
        m_fillColors.push_back(fillColor);
        m_strokeColors.push_back(strokeColor);
    }

    void ShapeBuilder::bezierVertex(float controlX1, float controlY1, float controlX2, float controlY2, float endX, float endY)
    {
        if (not requireBuilding("bezierVertex")) {
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
        if (not requireBuilding("quadraticVertex")) {
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
        if (not requireBuilding("curveVertex")) {
            return;
        }

        // Sliding 4-point window: drop the oldest point, append the new one at the end. After at
        // least 4 calls this shape, m_curvePoints holds exactly the last 4 pushed points in order.
        m_curvePoints[0] = m_curvePoints[1];
        m_curvePoints[1] = m_curvePoints[2];
        m_curvePoints[2] = m_curvePoints[3];
        m_curvePoints[3] = {x, y};
        ++m_curvePointCount;

        if (m_curvePointCount < 4) {
            return;
        }

        const float2& p0 = m_curvePoints[0];
        const float2& p1 = m_curvePoints[1];
        const float2& p2 = m_curvePoints[2];
        const float2& p3 = m_curvePoints[3];

        const float tangentScale = (1.0f - tightness) / 6.0f;
        const float2 controlPoint1 = p1 + (p2 - p0) * tangentScale;
        const float2 controlPoint2 = p2 - (p3 - p1) * tangentScale;

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
