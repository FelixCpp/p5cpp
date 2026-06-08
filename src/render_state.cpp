#include "render_state.hpp"

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
