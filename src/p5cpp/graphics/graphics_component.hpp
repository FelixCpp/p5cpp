#pragma once

#include <p5cpp/graphics/matrix_stack.hpp>
#include <p5cpp/graphics/renderer.hpp>
#include <p5cpp/graphics/render_state_stack.hpp>
#include <p5cpp/graphics/framebuffer.hpp>
#include <p5cpp/graphics/shaping.hpp>
#include <p5cpp/graphics/text.hpp>

namespace p5cpp
{
    class GraphicsComponent
    {
    public:
        explicit GraphicsComponent(uint32_t width, uint32_t height);

        void beginFrame();
        void endFrame();

        void resizeDefaultCanvas(uint32_t width, uint32_t height);
        void blitDefaultCanvasToScreen(uint32_t screenWidth, uint32_t screenHeight);

        void pushCanvas(Framebuffer framebuffer);
        void popCanvas();
        uint2 getCanvasSize();

        void pushState();
        void popState();

        void pushMatrix();
        void popMatrix();
        void resetMatrix();
        matrix4x4& peekMatrix();
        void applyMatrix(const matrix4x4& matrix);
        void setMatrix(const matrix4x4& matrix);
        void translate(float x, float y);
        void scale(float x, float y);
        void rotate(float radians);

        void fill(color_t color);
        void noFill();
        void stroke(color_t color);
        void noStroke();
        void strokeWeight(float strokeWeight);
        void strokeCap(StrokeCap strokeCap);
        void strokeJoin(StrokeJoin strokeJoin);
        void miterLimit(float miterLimit);
        void roundJoinThreshold(float roundJoinThreshold);

        void tint(color_t color);
        void noTint();

        void bezierDetail(uint32_t detail);
        void curveTightness(float tightness);
        void curveDetail(uint32_t detail);

        void textFont(const Font& font);
        void noTextFont();
        void textSize(float size);
        void textLetterSpacing(float letterSpacing);
        void textLineSpacing(float lineSpacing);
        void textAlign(TextAlign align);
        void textWrap(TextWrap wrap);

        void shader(const Shader& shader);
        void noShader();
        void blendMode(BlendMode blendMode);

        void setUniform(const std::string& name, const UniformVariable& variable);
        void setUniform(const Shader& shader, const std::string& name, const UniformVariable& variable);

        RenderState& peekRenderState();

        void background(color_t color);
        void rect(float left, float top, float width, float height);
        void rect(float left, float top, float width, float height, BorderRadius borderRadius);
        void ellipse(float centerX, float centerY, float radiusX, float radiusY);
        void triangle(float x1, float y1, float x2, float y2, float x3, float y3);
        void point(float centerX, float centerY);
        void line(float x1, float y1, float x2, float y2);
        void arc(float centerX, float centerY, float width, float height, float startAngle, float sweepAngle, ArcMode arcMode);
        void bezier(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4);
        void curve(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4);
        void image(const Texture& texture, float left, float top, float width, float height);
        void text(std::string_view text, float x, float y);

        void beginShape();
        void endShape(ShapeType type, bool close);
        void vertex(float x, float y, float u, float v);
        void curveVertex(float x, float y);

    private:
        void endShapeImpl(ShapeType type, bool close, const RenderState& renderState);

        Shader getShader(const RenderState& renderState);

        std::unique_ptr<float2[]> m_drawPointPositions;
        std::unique_ptr<float2[]> m_drawPointTexCoords;
        std::unique_ptr<color_t[]> m_drawPointFillColors;
        std::unique_ptr<color_t[]> m_drawPointStrokeColors;

        size_t m_drawPointCount;
        size_t m_drawPointCapacity;

        std::array<float2, 4> m_curveVertexPositions;
        size_t m_curveVertexCount;

        std::vector<Framebuffer> m_framebufferStack;
        Framebuffer m_defaultFramebuffer;

        RenderStateStack m_renderStateStack;
        MatrixStack m_matrixStack;
        Shader m_defaultShader;
        Shader m_textShader;
        Texture m_whiteTexture;
        UniformCache m_uniformCache;
        std::unique_ptr<NativeRenderer> m_renderer;
    };
} // namespace p5cpp
