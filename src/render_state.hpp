#pragma once

#include "p5.hpp"
#include "matrix_stack.hpp"

#include <vector>

namespace p5
{
    struct RenderState
    {
        color_t fillColor;
        color_t strokeColor;
        color_t tintColor;

        float strokeWeight;
        StrokeCap strokeCap;
        StrokeJoin strokeJoin;
        float miterLimit;
        float roundJoinThreshold;

        BlendMode blendMode;

        MatrixStack metrics;

        bool isFillDisabled;
        bool isStrokeDisabled;

        uint32_t bezierDetail;
        float invBezierDetail;

        float curveTightness;
        uint32_t curveDetail;
        float invCurveDetail;

        std::vector<float> strokePatternSegments;
        float strokePatternOffset;

        std::shared_ptr<Font> font;
        float textSize;

        std::shared_ptr<Shader> shader;

        HorizontalTextAlign horizontalTextAlign;
        VerticalTextAlign verticalTextAlign;

        RenderState();
    };
} // namespace p5
