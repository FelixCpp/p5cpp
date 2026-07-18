#pragma once

#include <p5cpp/graphics/matrix_stack.hpp>
#include <p5cpp/graphics/renderer.hpp>
#include <p5cpp/graphics/render_state_stack.hpp>
#include <p5cpp/graphics/framebuffer.hpp>
#include <p5cpp/graphics/filter.hpp>
#include <p5cpp/graphics/shaping.hpp>
#include <p5cpp/graphics/text.hpp>
#include <p5cpp/graphics/text_layout.hpp>
#include <p5cpp/graphics/render_group.hpp>
#include <p5cpp/graphics/render_group_recorder.hpp>
#include <p5cpp/graphics/effects_renderer.hpp>

#include <array>
#include <functional>
#include <span>

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
        std::vector<color_t> loadPixels();

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
        void textToPointsDetail(uint32_t detail);
        void textToPointsSpacing(float spacing);

        void shader(const Shader& shader);
        void noShader();
        void blendMode(BlendMode blendMode);

        void filter(FilterType type, float amount);
        void effect(const Shader& shader);

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
        void text(std::string_view text, float x, float y, float maxWidth);

        TextLayout layoutText(std::string_view text, float x, float y);
        TextLayout layoutText(std::string_view text, float x, float y, float maxWidth);

        std::vector<TextContour> textToPoints(std::string_view text, float x, float y);
        std::vector<TextContour> textToPoints(std::string_view text, float x, float y, float maxWidth);

        void beginShape();
        void endShape(ShapeType type, bool close);
        void vertex(float x, float y, float u, float v);
        void curveVertex(float x, float y);

        RenderGroup buildRenderGroup(const std::function<void()>& buildFn);
        void drawRenderGroup(const RenderGroup& group);

    private:
        void endShapeImpl(ShapeType type, bool close, const RenderState& renderState);

        Shader getShader(const RenderState& renderState);

        void submitFill(const PathPoints& pts, ShapeType type, const Texture& texture);
        void submitStroke(const PathPoints& pts, ShapeType type, bool close);

        // Every draw call reads state through these two funnels (never the raw stack
        // members below) so that buildRenderGroup() can redirect "the active state" to
        // its own isolated matrix/render-state stacks just by delegating to m_recorder —
        // no swapping/restoring of the concrete stack objects. See RenderGroupRecorder.
        MatrixStack& activeMatrixStack();
        RenderStateStack& activeRenderStateStack();

        // Analogous redirection for where tessellated/stroked triangles go: the live
        // renderer's frame buffer normally, or m_recorder's active recording sink while
        // inside buildRenderGroup().
        DrawBufferWriter& beginDrawOp();
        void endDrawOp(DrawBufferWriter& writer, const Shader& shader, const BlendMode& blendMode, const Texture& texture, std::span<const UniformSnapshot> uniforms);

        std::unique_ptr<float2[]> m_drawPointPositions;
        std::unique_ptr<float2[]> m_drawPointTexCoords;
        std::unique_ptr<color_t[]> m_drawPointFillColors;
        std::unique_ptr<color_t[]> m_drawPointStrokeColors;

        size_t m_drawPointCount;
        size_t m_drawPointCapacity;

        std::array<float2, 4> m_curveVertexPositions;
        size_t m_curveVertexCount;

        // Reusable per-shape scratch buffers: avoid a fresh heap allocation on every
        // ellipse()/rect()/bezier()/curve() call by keeping capacity across calls.
        std::vector<float2> m_ellipseFanPositions;
        std::vector<float2> m_ellipseFanUVs;
        std::vector<color_t> m_ellipseFillColors;
        std::vector<float2> m_ellipseStrokeUVs;
        std::vector<color_t> m_ellipseStrokeColors;

        std::vector<float2> m_roundedRectPositions;
        std::vector<float2> m_roundedRectUVs;
        std::vector<color_t> m_roundedRectFillColors;
        std::vector<color_t> m_roundedRectStrokeColors;

        std::vector<float2> m_curvePositions;
        std::vector<float2> m_curveUVs;
        std::vector<color_t> m_curveColors;

        // point() round-cap fan (center + perimeter, mirrors m_ellipseFan*).
        std::vector<float2> m_pointFanPositions;
        std::vector<float2> m_pointFanUVs;
        std::vector<color_t> m_pointFanColors;

        // arc() scratch buffers.
        std::vector<float2> m_arcPositions;
        std::vector<float2> m_arcFillPositions;
        std::vector<float2> m_arcFillUVs;
        std::vector<color_t> m_arcFillColors;
        std::vector<float2> m_arcStrokeUVs;
        std::vector<color_t> m_arcStrokeColors;
        std::vector<float2> m_arcPiePositions;
        std::vector<float2> m_arcPieUVs;
        std::vector<color_t> m_arcPieColors;

        std::vector<Framebuffer> m_framebufferStack;
        Framebuffer m_defaultFramebuffer;

        RenderStateStack m_renderStateStack;
        MatrixStack m_matrixStack;
        RenderGroupRecorder m_recorder;

        Shader m_defaultShader;
        Shader m_textShader;
        EffectsRenderer m_effects;
        Texture m_whiteTexture;
        UniformCache m_uniformCache;
        Font m_defaultFont;
        std::unique_ptr<NativeRenderer> m_renderer;
    };
} // namespace p5cpp
