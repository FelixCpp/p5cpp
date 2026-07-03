#include <p5cpp/application/engine.hpp>
#include <p5cpp/application/app_context.hpp>
#include <p5cpp/graphics/graphics_component.hpp>

namespace p5cpp
{
    extern std::unique_ptr<Engine> engine;
}

namespace p5cpp
{
    GraphicsComponent& getGraphicsComponent()
    {
        return engine->getContext().require<GraphicsComponent>();
    }
} // namespace p5cpp

namespace p5cpp
{
    void pushCanvas(Framebuffer framebuffer)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.pushCanvas(std::move(framebuffer));
    }

    void popCanvas()
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.popCanvas();
    }

    void pushState()
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.pushState();
    }

    void popState()
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.popState();
    }

    void pushMatrix()
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.pushMatrix();
    }

    void popMatrix()
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.popMatrix();
    }

    void resetMatrix()
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.resetMatrix();
    }

    void applyMatrix(const matrix4x4& matrix)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.applyMatrix(matrix);
    }

    void setMatrix(const matrix4x4& matrix)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.setMatrix(matrix);
    }

    void translate(float x, float y)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.translate(x, y);
    }

    void scale(float x, float y)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.scale(x, y);
    }

    void rotate(float radians)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.rotate(radians);
    }

    void fill(color_t color)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.fill(color);
    }

    void noFill()
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.noFill();
    }

    void stroke(color_t color)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.stroke(color);
    }

    void noStroke()
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.noStroke();
    }

    void strokeWeight(float strokeWeight)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.strokeWeight(strokeWeight);
    }

    void strokeCap(StrokeCap strokeCap)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.strokeCap(strokeCap);
    }

    void strokeJoin(StrokeJoin strokeJoin)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.strokeJoin(strokeJoin);
    }

    void miterLimit(float miterLimit)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.miterLimit(miterLimit);
    }

    void roundJoinThreshold(float roundJoinThreshold)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.roundJoinThreshold(roundJoinThreshold);
    }

    void tint(color_t color)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.tint(color);
    }

    void noTint()
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.noTint();
    }

    void bezierDetail(uint32_t detail)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.bezierDetail(detail);
    }

    void curveTightness(float tightness)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.curveTightness(tightness);
    }

    void curveDetail(uint32_t detail)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.curveDetail(detail);
    }

    void textFont(const Font& font)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.textFont(font);
    }

    void textSize(float size)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.textSize(size);
    }

    void textLetterSpacing(float letterSpacing)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.textLetterSpacing(letterSpacing);
    }

    void textLineSpacing(float lineSpacing)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.textLineSpacing(lineSpacing);
    }

    void textAlign(TextAlign align)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.textAlign(align);
    }

    void textWrap(TextWrap wrap)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.textWrap(wrap);
    }

    void shader(const Shader& shader)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.shader(shader);
    }

    void blendMode(BlendMode blendMode)
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        graphics.blendMode(blendMode);
    }

    RenderState& peekRenderState()
    {
        GraphicsComponent& graphics = getGraphicsComponent();
        return graphics.peekRenderState();
    }
} // namespace p5cpp
