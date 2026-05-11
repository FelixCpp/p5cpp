#include "renderstate.hpp"

namespace p5
{
    RenderState::RenderState()
        : fillColor(color(255, 255, 255)),
          strokeColor(color(255, 255, 255)),
          tintColor(color(255, 255, 255)),
          strokeWeight(1.0f),
          strokeCap(StrokeCap::round),
          strokeJoin(StrokeJoin::round),
          miterLimit(10.0f),
          roundJoinThreshold(0.44f), // 25°
          blendMode(BlendMode::alpha),
          metrics(),
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
          shader()
    {
    }
} // namespace p5

namespace p5
{
    RenderStateStack::RenderStateStack()
        : m_stack({RenderState {}}), m_activeRenderStateIndex(0)
    {
    }

    void RenderStateStack::push()
    {
        m_stack.push_back(m_stack[m_activeRenderStateIndex]);
        m_activeRenderStateIndex++;
    }

    void RenderStateStack::pop()
    {
        // Avoid popping the last render state to ensure there's always a valid state to work with
        if (m_activeRenderStateIndex > 0) {
            m_stack.pop_back();
            m_activeRenderStateIndex--;
        }
    }

    RenderState& RenderStateStack::peek()
    {
        return m_stack.at(m_activeRenderStateIndex);
    }

    void RenderStateStack::clear()
    {
        // Remove every stack except the first one to reset to the default state
        m_stack.erase(m_stack.begin() + 1, m_stack.end());
        m_activeRenderStateIndex = 0;
    }
} // namespace p5
