#include <p5cpp/graphics/shape_builder.hpp>

#include <cmath>
#include <algorithm>

namespace p5
{
    int curveSegmentCount(float controlPolygonLength)
    {
        return std::clamp(static_cast<int>(std::ceil(controlPolygonLength / 3.0f)), 8, 128);
    }
} // namespace p5

namespace p5
{
    ShapeBuilder::ShapeBuilder()
        : m_isBuilding(false),
          m_mode(ShapeMode::points),
          m_positions(std::make_unique<float2[]>(4)),
          m_texCoords(std::make_unique<float2[]>(4)),
          m_fillColors(std::make_unique<color_t[]>(4)),
          m_strokeColors(std::make_unique<color_t[]>(4)),
          m_vertexCount(0),
          m_vertexCapacity(4)
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
        m_vertexCount = 0;
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

        const std::span<const float2> positions {m_positions.get(), m_vertexCount};
        const std::span<const float2> texCoords {m_texCoords.get(), m_vertexCount};
        const std::span<const color_t> fillColors {m_fillColors.get(), m_vertexCount};
        const std::span<const color_t> strokeColors {m_strokeColors.get(), m_vertexCount};

        return BuiltShape {
            .mode = m_mode,
            .vertexCount = m_vertexCount,
            .positions = positions,
            .texCoords = texCoords,
            .fillColors = fillColors,
            .strokeColors = strokeColors
        };
    }

    void ShapeBuilder::vertex(float x, float y, color_t fillColor, color_t strokeColor)
    {
        vertex(x, y, 0.0f, 0.0f, fillColor, strokeColor);
    }

    void ShapeBuilder::vertex(float x, float y, float u, float v, color_t fillColor, color_t strokeColor)
    {
        if (not m_isBuilding) {
            throw std::runtime_error("Not building a shape");
        }

        if (m_vertexCount >= m_vertexCapacity) {
            const auto vertexCapacity = std::max(static_cast<size_t>(4), m_vertexCapacity * 2);
            auto positions = std::make_unique<float2[]>(vertexCapacity);
            auto texCoords = std::make_unique<float2[]>(vertexCapacity);
            auto fillColors = std::make_unique<color_t[]>(vertexCapacity);
            auto strokeColors = std::make_unique<color_t[]>(vertexCapacity);

            std::copy(m_positions.get(), m_positions.get() + m_vertexCount, positions.get());
            std::copy(m_texCoords.get(), m_texCoords.get() + m_vertexCount, texCoords.get());
            std::copy(m_fillColors.get(), m_fillColors.get() + m_vertexCount, fillColors.get());
            std::copy(m_strokeColors.get(), m_strokeColors.get() + m_vertexCount, strokeColors.get());

            m_positions = std::move(positions);
            m_texCoords = std::move(texCoords);
            m_fillColors = std::move(fillColors);
            m_strokeColors = std::move(strokeColors);
            m_vertexCapacity = vertexCapacity;
        }

        m_positions[m_vertexCount] = {x, y};
        m_texCoords[m_vertexCount] = {u, v};
        m_fillColors[m_vertexCount] = fillColor;
        m_strokeColors[m_vertexCount] = strokeColor;
        ++m_vertexCount;
    }

    void ShapeBuilder::bezierVertex(float controlX1, float controlY1, float controlX2, float controlY2, float endX, float endY)
    {
        if (not m_isBuilding) {
            throw std::runtime_error("Not building a shape");
        }

        if (m_vertexCount == 0) {
            throw std::runtime_error("No starting vertex for bezierVertex");
        }

        const float2 startPoint = m_positions[m_vertexCount - 1];
        const float2 controlPoint1 = {controlX1, controlY1};
        const float2 controlPoint2 = {controlX2, controlY2};
        const float2 endPoint = {endX, endY};

        const float length = distance(startPoint, controlPoint1) + distance(controlPoint1, controlPoint2) + distance(controlPoint2, endPoint);
        const int segments = curveSegmentCount(length);
        for (int i = 1; i <= segments; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(segments);
            const float u = 1.0f - t;
            const float2 point = u * u * u * startPoint + 3.0f * u * u * t * controlPoint1 + 3.0f * u * t * t * controlPoint2 + t * t * t * endPoint;
            vertex(point.x, point.y, m_texCoords[m_vertexCount - 1].x, m_texCoords[m_vertexCount - 1].y, m_fillColors[m_vertexCount - 1], m_strokeColors[m_vertexCount - 1]);
        }
    }

    void ShapeBuilder::quadraticVertex(float controlX, float controlY, float endX, float endY)
    {
        if (not m_isBuilding) {
            throw std::runtime_error("Not building a shape");
        }

        if (m_vertexCount == 0) {
            throw std::runtime_error("No starting vertex for quadraticVertex");
        }

        const float2 startPoint = m_positions[m_vertexCount - 1];
        const float2 controlPoint = {controlX, controlY};
        const float2 endPoint = {endX, endY};

        const float length = distance(startPoint, controlPoint) + distance(controlPoint, endPoint);
        const int segments = curveSegmentCount(length);
        for (int i = 1; i <= segments; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(segments);
            const float u = 1.0f - t;
            const float2 point = u * u * startPoint + 2.0f * u * t * controlPoint + t * t * endPoint;
            vertex(point.x, point.y, m_texCoords[m_vertexCount - 1].x, m_texCoords[m_vertexCount - 1].y, m_fillColors[m_vertexCount - 1], m_strokeColors[m_vertexCount - 1]);
        }
    }

    void ShapeBuilder::curveVertex(float x, float y, float tightness, color_t fillColor, color_t strokeColor)
    {
        if (not m_isBuilding) {
            throw std::runtime_error("Not building a shape");
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
        if (m_vertexCount == 0) {
            vertex(p1.x, p1.y, fillColor, strokeColor);
        }

        const float length = distance(p1, controlPoint1) + distance(controlPoint1, controlPoint2) + distance(controlPoint2, p2);
        const int segments = curveSegmentCount(length);
        for (int i = 1; i <= segments; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(segments);
            const float u = 1.0f - t;
            const float2 point = u * u * u * p1 + 3.0f * u * u * t * controlPoint1 + 3.0f * u * t * t * controlPoint2 + t * t * t * p2;
            vertex(point.x, point.y, m_texCoords[m_vertexCount - 1].x, m_texCoords[m_vertexCount - 1].y, m_fillColors[m_vertexCount - 1], m_strokeColors[m_vertexCount - 1]);
        }
    }
} // namespace p5
