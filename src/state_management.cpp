#include <p5cpp/p5cpp.hpp>
#include <p5cpp/application/app_context.hpp>

#include "application/engine.hpp"
#include "modules/rendering/rendering_data.hpp"
#include "render_state_stack.hpp"
#include "services/uniform_cache.hpp"

namespace p5cpp
{
    extern std::unique_ptr<Engine> engine;
}

namespace p5cpp
{
    inline RenderStateStack& getRenderStateStack()
    {
        return engine->getContext().require<RenderingData>().renderStateStack;
    }

    inline RenderState& getRenderState()
    {
        return render_state_stack_peek(getRenderStateStack());
    }
} // namespace p5cpp

namespace p5cpp
{
    void pushState()
    {
        render_state_stack_push(getRenderStateStack(), getRenderState());
    }

    void popState()
    {
        render_state_stack_pop(getRenderStateStack());
    }

    void pushMatrix()
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.metrics.push(renderState.metrics.peek());
    }

    void popMatrix()
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.metrics.pop();
    }

    void resetMatrix()
    {
        setMatrix(matrix4x4::identity);
    }

    void applyMatrix(const matrix4x4& matrix)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        matrix4x4& currentMatrix = renderState.metrics.peek();
        currentMatrix *= matrix;
    }

    void setMatrix(const matrix4x4& matrix)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        matrix4x4& currentMatrix = renderState.metrics.peek();
        currentMatrix = matrix;
    }

    matrix4x4& peekMatrix()
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        return renderState.metrics.peek();
    }

    void translate(float x, float y)
    {
        applyMatrix(matrix4x4::translation(x, y));
    }

    void scale(float x, float y)
    {
        applyMatrix(matrix4x4::scaling(x, y));
    }

    void rotate(float radians)
    {
        applyMatrix(matrix4x4::rotation(radians));
    }

    void noFill()
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.isFillDisabled = true;
    }

    void fill(int grey, int alpha) { fill(rgba(grey, grey, grey, alpha)); }
    void fill(int red, int green, int blue, int alpha) { fill(rgba(red, green, blue, alpha)); }
    void fill(color_t color)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.fillColor = color;
        renderState.isFillDisabled = false;
    }

    void noStroke()
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.isStrokeDisabled = true;
    }

    void stroke(int grey, int alpha) { stroke(rgba(grey, grey, grey, alpha)); }
    void stroke(int red, int green, int blue, int alpha) { stroke(rgba(red, green, blue, alpha)); }
    void stroke(color_t color)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.strokeColor = color;
        renderState.isStrokeDisabled = false;
    }

    void strokeWeight(float strokeWeight)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.strokeWeight = strokeWeight;
    }

    void strokeCap(StrokeCap strokeCap)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.strokeCap = strokeCap;
    }

    void strokeJoin(StrokeJoin strokeJoin)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.strokeJoin = strokeJoin;
    }

    void miterLimit(float miterLimit)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.miterLimit = miterLimit;
    }

    void roundJoinThreshold(float angleThreshold)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.roundJoinThreshold = angleThreshold;
    }

    void blendMode(BlendMode blendMode)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.blendMode = blendMode;
    }

    void curveTightness(float tightness)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.curveTightness = tightness;
    }

    void curveDetail(uint32_t detail)
    {
        detail = std::max(detail, 1u);

        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.curveDetail = detail;
        renderState.invCurveDetail = 1.0f / static_cast<float>(detail);
    }

    void bezierDetail(uint32_t detail)
    {
        detail = std::max(detail, 1u);

        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.bezierDetail = detail;
        renderState.invBezierDetail = 1.0f / static_cast<float>(detail);
    }

    void shader(std::shared_ptr<Shader> shader)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.shader = std::move(shader);
    }

    void noShader()
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.shader.reset();
    }

    void setUniform(const std::string& name, const UniformVariable& variable)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        if (renderState.shader != nullptr) {
            setUniform(renderState.shader, name, variable);
        }
    }

    void setUniform(std::shared_ptr<Shader> shader, const std::string& name, const UniformVariable& variable)
    {
        UniformCache* uniformCache = engine->getContext().require<RenderingData>().uniformCache.get();
        uniformCache->setUniform(shader.get(), name, variable);
    }

    void textAlign(TextAlign textAlign)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.textAlign = textAlign;
    }

    void textWrap(TextWrap textWrap)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.textWrap = textWrap;
    }

    void textFont(std::shared_ptr<Font> font)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.font = font;
    }

    void noTextFont()
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.font.reset();
    }

    void textSize(float size)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.textSize = size;
    }

    void textLetterSpacing(float spacing)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.textLetterSpacing = spacing;
    }

    void textLineSpacing(float spacing)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.textLineSpacing = spacing;
    }

    void tint(int grey, int alpha) { tint(rgba(grey, grey, grey, alpha)); }
    void tint(int red, int green, int blue, int alpha) { tint(rgba(red, green, blue, alpha)); }
    void tint(color_t color)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.tintColor = color;
    }

    void noTint()
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        renderState.tintColor = rgba(255, 255, 255);
    }
} // namespace p5cpp
