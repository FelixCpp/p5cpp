#include "renderstate.hpp"

namespace p5
{
    RenderState::RenderState()
        : fillColor(rgba(255, 255, 255)),
          strokeColor(rgba(255, 255, 255)),
          tintColor(rgba(255, 255, 255)),
          strokeWeight(1.0f),
          strokeCap(StrokeCap::round),
          strokeJoin(StrokeJoin::round),
          miterLimit(10.0f),
          roundJoinThreshold(0.44f), // 25°
          blendMode(BlendMode::alpha),
          metrics(matrix_stack_create()),
          isFillDisabled(false),
          isStrokeDisabled(false),
          bezierDetail(20),
          invBezierDetail(1.0f / 20.0f),
          curveTightness(0.0f),
          curveDetail(20),
          invCurveDetail(1.0f / 20.0f),
          strokePatternSegments(),
          strokePatternOffset(0.0f),
          font(),
          textSize(12.0f),
          shader(),
          horizontalTextAlign(HorizontalTextAlign::left),
          verticalTextAlign(VerticalTextAlign::baseline)
    {
    }
} // namespace p5

namespace p5
{
    RenderStateStack render_state_stack_create()
    {
        return RenderStateStack {
            .renderStates = {RenderState()},
            .activeRenderStateIndex = 0
        };
    }

    void render_state_stack_push(RenderStateStack& stack, const RenderState& state)
    {
        if (stack.activeRenderStateIndex < stack.renderStates.size() - 1) {
            stack.activeRenderStateIndex++;
            stack.renderStates[stack.activeRenderStateIndex] = state;
        } else {
            stack.renderStates.push_back(state);
            stack.activeRenderStateIndex = stack.renderStates.size() - 1;
        }
    }

    void render_state_stack_pop(RenderStateStack& stack)
    {
        if (stack.activeRenderStateIndex > 0) {
            stack.activeRenderStateIndex--;
        }
    }

    void render_state_stack_clear(RenderStateStack& stack)
    {
        stack.renderStates.resize(1);
        stack.renderStates[0] = RenderState();
        stack.activeRenderStateIndex = 0;
    }

    RenderState& render_state_stack_peek(RenderStateStack& stack)
    {
        return stack.renderStates[stack.activeRenderStateIndex];
    }
} // namespace p5
