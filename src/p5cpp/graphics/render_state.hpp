#pragma once

#include <p5cpp/graphics/color.hpp>
#include <p5cpp/graphics/shader.hpp>
#include <p5cpp/graphics/text.hpp>
#include <p5cpp/graphics/shaping.hpp>
#include <p5cpp/graphics/font.hpp>
#include <p5cpp/graphics/blendmode.hpp>

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
    };

    RenderState render_state_create();
} // namespace p5cpp
