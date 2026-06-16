#pragma once

#include <p5cpp.hpp>

#include "matrix_stack.hpp"

namespace p5cpp
{
    struct RenderState
    {
        color_t fillColor;
        bool isFillDisabled;

        color_t strokeColor;
        bool isStrokeDisabled;
        float strokeWeight;
        StrokeCap strokeCap;
        StrokeJoin strokeJoin;
        float miterLimit;
        float roundJoinThreshold;

        color_t tintColor;

        uint32_t bezierDetail;
        float invBezierDetail;

        float curveTightness;
        uint32_t curveDetail;
        float invCurveDetail;

        std::shared_ptr<Font> font;
        float textSize;
        float textLetterSpacing;
        float textLineSpacing;
        TextAlign textAlign;
        TextWrap textWrap;

        std::shared_ptr<Shader> shader;
        BlendMode blendMode;
        MatrixStack metrics;
    };

    RenderState render_state_create();
} // namespace p5cpp
