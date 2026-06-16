#include "render_state.hpp"
#include <p5cpp.hpp>

namespace p5cpp
{
    RenderState render_state_create()
    {
        return {
            .fillColor = rgba(255, 255, 255),
            .isFillDisabled = false,
            .strokeColor = rgba(255, 255, 255),
            .isStrokeDisabled = false,
            .strokeWeight = 1.0f,
            .strokeCap = StrokeCap::round,
            .strokeJoin = StrokeJoin::miter,
            .miterLimit = 10.0f,
            .roundJoinThreshold = radians(25.0f),
            .tintColor = rgba(255, 255, 255),
            .bezierDetail = 20,
            .invBezierDetail = 1.0f / 20.0f,
            .curveTightness = 0.0f,
            .curveDetail = 20,
            .invCurveDetail = 1.0f / 20.0f,
            .font = nullptr,
            .textSize = 12.0f,
            .textLetterSpacing = 0.0f,
            .textLineSpacing = 1.0f,
            .textAlign = TextAlign {.horizontal = HorizontalTextAlign::left, .vertical = VerticalTextAlign::baseline},
            .textWrap = TextWrap::none,
            .shader = nullptr,
            .blendMode = BlendMode::alpha,
            .metrics = matrix_stack_create(),
        };
    }
} // namespace p5cpp
