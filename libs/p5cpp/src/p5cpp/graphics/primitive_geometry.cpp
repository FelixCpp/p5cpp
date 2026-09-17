#include <p5cpp/graphics/primitive_geometry.hpp>
#include <p5cpp/graphics/curve_tessellation.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace p5::detail
{
    int ellipseSegmentCount(float radiusX, float radiusY)
    {
        const float maxRadius = std::max(std::abs(radiusX), std::abs(radiusY));
        return segmentCountForArcLength(std::numbers::pi_v<float> * std::sqrt(2.0f * maxRadius), 1.0f, 16, 256);
    }

    void buildEllipsePoints(float centerX, float centerY, float radiusX, float radiusY, std::vector<float2>& positions, std::vector<float2>& texCoords)
    {
        const int segments = ellipseSegmentCount(radiusX, radiusY);
        positions.resize(segments);
        texCoords.resize(segments);

        for (int i = 0; i < segments; ++i) {
            const float angle = (2.0f * std::numbers::pi_v<float> * static_cast<float>(i)) / static_cast<float>(segments);
            const float c = std::cos(angle);
            const float s = std::sin(angle);
            positions[i] = {centerX + c * radiusX, centerY + s * radiusY};
            texCoords[i] = {0.5f + 0.5f * c, 0.5f + 0.5f * s};
        }
    }

    int arcSegmentCount(float radiusX, float radiusY, float angleSpan)
    {
        constexpr float twoPi = 2.0f * std::numbers::pi_v<float>;
        const float fraction = std::clamp(std::abs(angleSpan) / twoPi, 0.0f, 1.0f);
        const float approxSegments = static_cast<float>(ellipseSegmentCount(radiusX, radiusY)) * fraction;
        return segmentCountForArcLength(approxSegments, 1.0f, 2, 256);
    }

    void buildArcPoints(float centerX, float centerY, float radiusX, float radiusY, float startAngle, float stopAngle, std::vector<float2>& positions, std::vector<float2>& texCoords)
    {
        const int segments = arcSegmentCount(radiusX, radiusY, stopAngle - startAngle);
        positions.resize(segments + 1);
        texCoords.resize(segments + 1);

        for (int i = 0; i <= segments; ++i) {
            const float angle = std::lerp(startAngle, stopAngle, static_cast<float>(i) / static_cast<float>(segments));
            const float c = std::cos(angle);
            const float s = std::sin(angle);
            positions[i] = {centerX + c * radiusX, centerY + s * radiusY};
            texCoords[i] = {0.5f + 0.5f * c, 0.5f + 0.5f * s};
        }
    }

    void buildRoundedRectPoints(float left, float top, float width, float height, const BorderRadius& borderRadius, color_t fillColor, color_t strokeColor, const matrix4x4& transform, ShapeBuilder& builder)
    {
        const float right = left + width;
        const float bottom = top + height;
        const float halfWidth = width * 0.5f;
        const float halfHeight = height * 0.5f;

        const auto clampCorner = [&](const CornerRadius& corner) -> float2 {
            return {std::clamp(corner.radiusX, 0.0f, halfWidth), std::clamp(corner.radiusY, 0.0f, halfHeight)};
        };

        const float2 topLeftRadius = clampCorner(borderRadius.topLeft);
        const float2 topRightRadius = clampCorner(borderRadius.topRight);
        const float2 bottomRightRadius = clampCorner(borderRadius.bottomRight);
        const float2 bottomLeftRadius = clampCorner(borderRadius.bottomLeft);

        const auto addCorner = [&](float cornerX, float cornerY, float centerX, float centerY, float radiusX, float radiusY, float startAngle, float endAngle) {
            const auto addVertex = [&](float x, float y) {
                const float2 transformed = transformPoint(transform, {x, y});
                builder.vertex(transformed.x, transformed.y, (x - left) / width, (y - top) / height, fillColor, strokeColor);
            };

            if (radiusX <= 0.0f or radiusY <= 0.0f) {
                addVertex(cornerX, cornerY);
                return;
            }

            const int segments = std::max(ellipseSegmentCount(radiusX, radiusY) / 4, 2);
            for (int i = 0; i <= segments; ++i) {
                const float t = std::lerp(startAngle, endAngle, static_cast<float>(i) / static_cast<float>(segments));
                addVertex(centerX + std::cos(t) * radiusX, centerY + std::sin(t) * radiusY);
            }
        };

        constexpr float pi = std::numbers::pi_v<float>;

        addCorner(left, top, left + topLeftRadius.x, top + topLeftRadius.y, topLeftRadius.x, topLeftRadius.y, pi, 1.5f * pi);
        addCorner(right, top, right - topRightRadius.x, top + topRightRadius.y, topRightRadius.x, topRightRadius.y, 1.5f * pi, 2.0f * pi);
        addCorner(right, bottom, right - bottomRightRadius.x, bottom - bottomRightRadius.y, bottomRightRadius.x, bottomRightRadius.y, 0.0f, 0.5f * pi);
        addCorner(left, bottom, left + bottomLeftRadius.x, bottom - bottomLeftRadius.y, bottomLeftRadius.x, bottomLeftRadius.y, 0.5f * pi, pi);
    }
} // namespace p5::detail
