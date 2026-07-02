#include <p5cpp/graphics/primitive_renderer.hpp>

namespace p5cpp
{
    PrimitiveRenderer::PrimitiveRenderer()
        : curveVertexCount(0)
    {
    }

    void PrimitiveRenderer::beginShape()
    {
    }

    void PrimitiveRenderer::endShape(ShapeType type, bool close, const RenderState& renderState)
    {
    }

    void PrimitiveRenderer::vertex(float x, float y, float u, float v, const RenderState& renderState)
    {
        if (drawPointCount >= drawPointCapacity) {
            const size_t newCapacity = std::max(drawPointCount * 2uz, 4uz);

            std::unique_ptr<float2[]> newPositions = std::make_unique<float2[]>(newCapacity);
            std::unique_ptr<float2[]> newTexCoords = std::make_unique<float2[]>(newCapacity);
            std::unique_ptr<color_t[]> newFillColors = std::make_unique<color_t[]>(newCapacity);
            std::unique_ptr<color_t[]> newStrokeColors = std::make_unique<color_t[]>(newCapacity);

            if (drawPointCount > 0) {
                std::copy(drawPointPositions.get(), drawPointPositions.get() + drawPointCount, newPositions.get());
                std::copy(drawPointTexCoords.get(), drawPointTexCoords.get() + drawPointCount, newTexCoords.get());
                std::copy(drawPointFillColors.get(), drawPointFillColors.get() + drawPointCount, newFillColors.get());
                std::copy(drawPointStrokeColors.get(), drawPointStrokeColors.get() + drawPointCount, newStrokeColors.get());
            }

            drawPointPositions = std::move(newPositions);
            drawPointTexCoords = std::move(newTexCoords);
            drawPointFillColors = std::move(newFillColors);
            drawPointStrokeColors = std::move(newStrokeColors);
            drawPointCapacity = newCapacity;
        }

        const matrix4x4& matrix = renderState.metrics.peek();
        const float2 transformedPosition = matrix.transformPoint(x, y);

        drawPointPositions[drawPointCount] = transformedPosition;
        drawPointTexCoords[drawPointCount] = float2 {u, v};
        drawPointFillColors[drawPointCount] = renderState.fillColor;
        drawPointStrokeColors[drawPointCount] = renderState.strokeColor;
        ++drawPointCount;

        curveVertexCount = 0; // Reset curve vertex count when a regular vertex is added
    }

    void PrimitiveRenderer::curveVertex(float x, float y, const RenderState& renderState)
    {
        curveVertexPositions.at(curveVertexCount++) = float2 {x, y};

        if (curveVertexCount == curveVertexPositions.size()) {
            const float alpha = (1.0f - renderState.curveTightness) * 0.5f;

            auto [x1, y1] = curveVertexPositions[0];
            auto [x2, y2] = curveVertexPositions[1];
            auto [x3, y3] = curveVertexPositions[2];
            auto [x4, y4] = curveVertexPositions[3];

            for (size_t j = 0; j <= renderState.curveDetail; ++j) {
                float t = static_cast<float>(j) * renderState.invCurveDetail;
                float t2 = t * t;
                float t3 = t2 * t;

                float bx = alpha * ((-x1 + 3 * x2 - 3 * x3 + x4) * t3 + (2 * x1 - 5 * x2 + 4 * x3 - x4) * t2 + (-x1 + x3) * t) + x2;
                float by = alpha * ((-y1 + 3 * y2 - 3 * y3 + y4) * t3 + (2 * y1 - 5 * y2 + 4 * y3 - y4) * t2 + (-y1 + y3) * t) + y2;

                vertex(bx, by, 0.0f, 0.0f, renderState);
            }

            curveVertexCount = 0;
        }
    }

    void PrimitiveRenderer::rect(float left, float top, float width, float height, const RenderState& renderState)
    {
        beginShape();
        {
            vertex(left, top, 0.0f, 0.0f, renderState);
            vertex(left + width, top, 0.0f, 0.0f, renderState);
            vertex(left + width, top + height, 0.0f, 0.0f, renderState);
            vertex(left, top + height, 0.0f, 0.0f, renderState);
        }
        endShape(ShapeType::quads, true, renderState);
    }

    void PrimitiveRenderer::square(float left, float top, float size, const RenderState& renderState)
    {
        rect(left, top, size, size, renderState);
    }

    void PrimitiveRenderer::ellipse(float centerX, float centerY, float width, float height, size_t segments, const RenderState& renderState)
    {
        if (not renderState.isFillDisabled) {
            beginShape();
            vertex(width * 0.5f, height * 0.5f, 0.0f, 0.0f, renderState);
            {
                for (size_t i = 0; i < segments; ++i) {
                    float angle = static_cast<float>(i) / static_cast<float>(segments) * 2.0f * M_PI;
                    float x = centerX + (width * 0.5f) * std::cos(angle);
                    float y = centerY + (height * 0.5f) * std::sin(angle);
                    vertex(x, y, 0.0f, 0.0f, renderState);
                }
            }
            endShapeFillOnly(ShapeType::triangleFan, true, ColorChoice::fill, renderState);
        }

        if (not renderState.isStrokeDisabled) {
            beginShape();
            {
                for (size_t i = 0; i < segments; ++i) {
                    float angle = static_cast<float>(i) / static_cast<float>(segments) * 2.0f * M_PI;
                    float x = centerX + (width * 0.5f) * std::cos(angle);
                    float y = centerY + (height * 0.5f) * std::sin(angle);
                    vertex(x, y, 0.0f, 0.0f, renderState);
                }
            }
            endShapeStrokeOnly(ShapeType::lineLoop, true, ColorChoice::stroke, renderState);
        }
    }

    void PrimitiveRenderer::circle(float centerX, float centerY, float size, size_t segments, const RenderState& renderState)
    {
        ellipse(centerX, centerY, size, size, segments, renderState);
    }

    void PrimitiveRenderer::point(float x, float y, size_t segments, const RenderState& renderState)
    {
        const float radius = renderState.strokeWeight * 0.5f;

        beginShape();
        vertex(x, y, 0.0f, 0.0f, renderState);
        for (size_t i = 0; i < segments; ++i) {
            float angle = static_cast<float>(i) / static_cast<float>(segments) * 2.0f * M_PI;
            float px = x + radius * std::cos(angle);
            float py = y + radius * std::sin(angle);
            vertex(px, py, 0.0f, 0.0f, renderState);
        }
        endShapeStrokeAsFill(ShapeType::triangleFan, true, renderState);
    }

    PathPoints PrimitiveRenderer::buildFillDrawPoints() const
    {
        return PathPoints {
            .size = drawPointCount,
            .positions = std::span<const float2>(drawPointPositions.get(), drawPointCount),
            .texcoords = std::span<const float2>(drawPointTexCoords.get(), drawPointCount),
            .colors = std::span<const color_t>(drawPointFillColors.get(), drawPointCount),
        };
    }

    PathPoints PrimitiveRenderer::buildStrokeDrawPoints() const
    {
        return PathPoints {
            .size = drawPointCount,
            .positions = std::span<const float2>(drawPointPositions.get(), drawPointCount),
            .texcoords = std::span<const float2>(drawPointTexCoords.get(), drawPointCount),
            .colors = std::span<const color_t>(drawPointStrokeColors.get(), drawPointCount),
        };
    }
} // namespace p5cpp
