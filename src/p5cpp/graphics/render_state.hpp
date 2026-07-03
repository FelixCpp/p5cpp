#pragma once

#include <p5cpp/p5cpp.hpp> // TODO(Felix): This should be removed in the future

#include <p5cpp/graphics/color.hpp>
#include <p5cpp/graphics/matrix_stack.hpp>
#include <p5cpp/graphics/shader.hpp>

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

        std::optional<Font> font;
        float textSize;
        float textLetterSpacing;
        float textLineSpacing;
        TextAlign textAlign;
        TextWrap textWrap;

        std::optional<Shader> shader;
        BlendMode blendMode;
        MatrixStack metrics;
    };

    RenderState render_state_create();
} // namespace p5cpp
