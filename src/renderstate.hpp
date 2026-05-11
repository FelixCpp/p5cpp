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

        HorizontalTextAlign horizontalTextAlign = HorizontalTextAlign::left;
        VerticalTextAlign verticalTextAlign = VerticalTextAlign::baseline;

        RenderState();
    };
} // namespace p5

namespace p5
{
    class RenderStateStack
    {
    public:
        RenderStateStack();

        void push();
        void pop();
        RenderState& peek();

        void clear();

    private:
        std::vector<RenderState> m_stack;
        size_t m_activeRenderStateIndex;
    };
} // namespace p5
