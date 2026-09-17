#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/shape_builder.hpp>

#include <vector>

namespace p5::detail
{
    int ellipseSegmentCount(float radiusX, float radiusY);
    void buildEllipsePoints(float centerX, float centerY, float radiusX, float radiusY, std::vector<float2>& positions, std::vector<float2>& texCoords);

    int arcSegmentCount(float radiusX, float radiusY, float angleSpan);
    void buildArcPoints(float centerX, float centerY, float radiusX, float radiusY, float startAngle, float stopAngle, std::vector<float2>& positions, std::vector<float2>& texCoords);

    void buildRoundedRectPoints(float left, float top, float width, float height, const BorderRadius& borderRadius, color_t fillColor, color_t strokeColor, const matrix4x4& transform, ShapeBuilder& builder);
} // namespace p5::detail
