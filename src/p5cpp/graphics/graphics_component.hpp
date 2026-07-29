#pragma once

#include <p5cpp/graphics/matrix_stack.hpp>
#include <p5cpp/graphics/renderer.hpp>
#include <p5cpp/graphics/render_state_stack.hpp>
#include <p5cpp/graphics/framebuffer.hpp>
#include <p5cpp/graphics/antialiased_canvas.hpp>
#include <p5cpp/graphics/shaping.hpp>
#include <p5cpp/graphics/text.hpp>
#include <p5cpp/graphics/text_layout.hpp>
#include <p5cpp/graphics/render_group.hpp>
#include <p5cpp/graphics/render_group_recorder.hpp>
#include <p5cpp/graphics/pixels.hpp>
#include <p5cpp/graphics/mesh.hpp>

#include <array>
#include <functional>
#include <span>

namespace p5cpp
{
    class GraphicsComponent
    {
    public:
        explicit GraphicsComponent(uint32_t width, uint32_t height);

        // Frees every GL resource this component owns directly (m_defaultShader,
        // m_textShader, m_whiteTexture, m_canvas's framebuffers). Must be called
        // explicitly from GraphicsModule::destroy() while the GL context is still
        // current - GraphicsComponent has no destructor that does this, since by the
        // time it's actually destructed (via GraphicsModule's unique_ptr, torn down
        // when AppEngine::modules is destroyed) the GL context is already gone (see
        // WindowModule::destroy()'s window.reset(), which runs after every module
        // nested inside its next() call has already finished destroy()).
        void releaseGpuResources();

        void beginFrame();
        void endFrame();

        void resizeDefaultCanvas(uint32_t width, uint32_t height);
        void blitDefaultCanvasToScreen(uint32_t screenWidth, uint32_t screenHeight);

        // Enables MSAA on the default canvas: shapes are drawn into a multisample
        // target and resolved into the presented canvas once per frame. samples is
        // clamped to what the driver supports (GL_MAX_SAMPLES). Off by default.
        void smooth(uint32_t samples);
        void noSmooth();

        void pushCanvas(Framebuffer framebuffer);
        void popCanvas();
        uint2 getCanvasSize();
        Pixels loadPixels();
        void updatePixels(const Pixels& pixels);

        void pushState();
        void popState();
        void withState(const std::function<void()>& fn);

        void pushMatrix();
        void popMatrix();
        void resetMatrix();
        const matrix4x4& peekMatrix();
        void applyMatrix(const matrix4x4& matrix);
        void setMatrix(const matrix4x4& matrix);
        void translate(float x, float y);
        void scale(float x, float y);
        void rotate(float radians);

        void push();
        void pop();

        void fill(color_t color);
        void noFill();
        color_t getFillColor();
        bool isFillDisabled();
        void stroke(color_t color);
        void noStroke();
        color_t getStrokeColor();
        bool isStrokeDisabled();
        void strokeWeight(float strokeWeight);
        void strokeCap(StrokeCap strokeCap);
        void strokeJoin(StrokeJoin strokeJoin);
        void miterLimit(float miterLimit);
        void roundJoinThreshold(float roundJoinThreshold);
        float getStrokeWeight();
        StrokeCap getStrokeCap();
        StrokeJoin getStrokeJoin();
        float getMiterLimit();
        float getRoundJoinThreshold();

        void tint(color_t color);
        void noTint();
        color_t getTintColor();

        void textureMode(TextureMode textureMode);
        TextureMode getTextureMode();

        void bezierDetail(uint32_t detail);
        void curveTightness(float tightness);
        void curveDetail(uint32_t detail);
        uint32_t getBezierDetail();
        float getCurveTightness();
        uint32_t getCurveDetail();

        void textFont(const Font& font);
        void noTextFont();
        void textSize(float size);
        void textLetterSpacing(float letterSpacing);
        void textLineSpacing(float lineSpacing);
        void textAlign(TextAlign align);
        void textWrap(TextWrap wrap);
        void textToPointsDetail(uint32_t detail);
        void textToPointsSpacing(float spacing);
        Font getTextFont();
        float getTextSize();
        float getTextLetterSpacing();
        float getTextLineSpacing();
        TextAlign getTextAlign();
        TextWrap getTextWrap();
        uint32_t getTextToPointsDetail();
        float getTextToPointsSpacing();

        void shader(const Shader& shader);
        void noShader();
        void blendMode(BlendMode blendMode);
        Shader getShader();
        BlendMode getBlendMode();

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
        void image(const Texture& texture, float left, float top, float width, float height, float sx, float sy, float sWidth, float sHeight);

        void mesh(std::span<const MeshVertex> vertices, std::span<const uint32_t> indices);
        void mesh(std::span<const MeshVertex> vertices, std::span<const uint32_t> indices, const Texture& texture);

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
        // Framebuffer::readPixels()/writePixels() use raw bottom-to-top GL row order;
        // Pixels exposes top-left origin like the rest of the API. This flips between them.
        static std::vector<color_t> flipRows(std::span<const color_t> src, uint32_t width, uint32_t height);

        void endShapeImpl(ShapeType type, bool close, const RenderState& renderState);

        // Swaps whichever canvas beginFrame() would push (m_canvas.activeFramebuffer())
        // into the still-open outermost stack bracket. See resizeDefaultCanvas() for why
        // this matters mid-bracket.
        void swapActiveDefaultCanvas(const Framebuffer& newDefaultCanvas);

        // Resolves m_canvas's multisampled target into its default framebuffer (no-op if
        // smooth() isn't active). Called once per frame in endFrame(), and on demand
        // before anything (loadPixels()) needs to read the canvas mid-frame while
        // smooth() is active.
        void resolveMsaaToDefaultFramebuffer();

        // Inverse of the above: pushes m_canvas's default framebuffer content back into
        // the live msaa target. Needed after updatePixels() mutates the default
        // framebuffer out-of-band while smooth() is active, so that drawing continues on
        // top of the mutated result instead of the next automatic resolve silently
        // overwriting it with the stale unmutated content.
        void syncMsaaFromDefaultFramebuffer();

        Shader getShader(const RenderState& renderState);

        // A run of shaped glyphs making up one visual (post-wrap) line, shared by
        // text() (which draws it) and layoutText() (which only measures it) so the
        // wrapping rules can't drift between the two.
        struct VisualLine
        {
            std::vector<ShapedGlyph> glyphs;
            float width = 0.0f;
        };

        static std::vector<VisualLine> shapeTextLines(const Font& font, std::string_view text, int textSizeInt, const RenderState& rs, float maxWidth);

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
        AntialiasedCanvas m_canvas;

        RenderStateStack m_renderStateStack;
        MatrixStack m_matrixStack;
        RenderGroupRecorder m_recorder;

        Shader m_defaultShader;
        Shader m_textShader;
        Texture m_whiteTexture;
        UniformCache m_uniformCache;
        Font m_defaultFont;
        std::unique_ptr<NativeRenderer> m_renderer;
    };
} // namespace p5cpp
