#pragma once

#include "p5.hpp"
#include "matrixstack.hpp"

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

namespace p5
{
    struct RenderStateStack
    {
        std::vector<RenderState> renderStates;
        size_t activeRenderStateIndex;
    };

    RenderStateStack render_state_stack_create();
    void render_state_stack_push(RenderStateStack& stack, const RenderState& state);
    void render_state_stack_pop(RenderStateStack& stack);
    void render_state_stack_clear(RenderStateStack& stack);
    RenderState& render_state_stack_peek(RenderStateStack& stack);
} // namespace p5
