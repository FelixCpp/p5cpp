#pragma once

#include <p5cpp/graphics/color.hpp>
#include <p5cpp/graphics/render_state.hpp>
#include <p5cpp/graphics/shaping.hpp>
#include <p5cpp/math/value2.hpp>

#include <array>
#include <memory>

namespace p5cpp
{
    class PrimitiveRenderer
    {
    public:
        PrimitiveRenderer();

        void beginShape();
        void endShape(ShapeType type, bool close, const RenderState& renderState);
        void vertex(float x, float y, float u, float v, const RenderState& renderState);
        void curveVertex(float x, float y, const RenderState& renderState);

        void rect(float left, float top, float width, float height, const RenderState& renderState);
        void square(float left, float top, float size, const RenderState& renderState);
        void ellipse(float centerX, float centerY, float width, float height, size_t segments, const RenderState& renderState);
        void circle(float centerX, float centerY, float size, size_t segments, const RenderState& renderState);
        void point(float x, float y, size_t segments, const RenderState& renderState);
        void line(float x1, float y1, float x2, float y2, const RenderState& renderState);

    private:
        enum ColorChoice {
            fill,
            stroke,
        };

        void endShapeFillOnly(ShapeType type, bool close, ColorChoice choice, const RenderState& renderState);
        void endShapeStrokeOnly(ShapeType type, bool close, ColorChoice choice, const RenderState& renderState);
        void endShapeStrokeAsFill(ShapeType type, bool close, const RenderState& renderState);

        PathPoints buildFillDrawPoints() const;
        PathPoints buildStrokeDrawPoints() const;

        size_t drawPointCount;
        size_t drawPointCapacity;

        std::unique_ptr<float2[]> drawPointPositions;
        std::unique_ptr<float2[]> drawPointTexCoords;
        std::unique_ptr<color_t[]> drawPointFillColors;
        std::unique_ptr<color_t[]> drawPointStrokeColors;

        std::array<float2, 4> curveVertexPositions;
        size_t curveVertexCount;
    };
} // namespace p5cpp
