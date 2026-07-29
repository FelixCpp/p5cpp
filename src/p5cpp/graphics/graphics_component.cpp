#include <p5cpp/graphics/dejavusans.hpp>
#include <p5cpp/graphics/internal_shaders.hpp>
#include <p5cpp/graphics/graphics_component.hpp>
#include <p5cpp/graphics/tess.hpp>
#include <p5cpp/graphics/stroker.hpp>
#include <p5cpp/math/constants.hpp>

#include <glad/glad.h>
#include <algorithm>
#include <cassert>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace p5cpp
{
    struct DefaultComputeCircleSegmentCount : public ComputeCircleSegmentCount
    {
        size_t operator()(float angle, float radius) const
        {
            const float error = 0.75f; // maximaler Fehler in Pixeln (tweakbar)

            if (radius <= 0.0f)
                return 0;

            // Clamp, um numerische Probleme zu vermeiden
            const float cosValue = 1.0f - (error / radius);
            const float clamped = std::clamp(cosValue, -1.0f, 1.0f);

            const float step = std::acos(clamped);

            if (step <= 0.0f)
                return 4;

            const size_t segments = static_cast<size_t>(std::ceil(std::abs(angle) / step));

            return std::max<size_t>(segments, 4);
        }
    };

    inline thread_local DefaultComputeCircleSegmentCount computeCircleSegmentCount;
} // namespace p5cpp

namespace p5cpp
{
    // Caches unit-circle sample points (cos, sin) so that repeated ellipse()/point()/
    // rounded-rect-corner calls that land on the same segment count (very common, since
    // computeCircleSegmentCount quantizes radius into a limited set of segment counts)
    // don't re-evaluate std::cos/std::sin for every point on every frame.
    class UnitCircleTableCache
    {
    public:
        // Full circle: points at angle = TWO_PI * i / segments, i = 0..segments (inclusive).
        const std::vector<float2>& getFullCircle(size_t segments)
        {
            return getOrBuild(m_fullCircleTables, segments, TWO_PI);
        }

        // Quarter circle: points at angle = HALF_PI * i / segments, i = 0..segments (inclusive).
        const std::vector<float2>& getQuarterCircle(size_t segments)
        {
            return getOrBuild(m_quarterCircleTables, segments, HALF_PI);
        }

    private:
        static const std::vector<float2>& getOrBuild(std::unordered_map<size_t, std::vector<float2>>& tables, size_t segments, float fullSweep)
        {
            const auto it = tables.find(segments);
            if (it != tables.end())
                return it->second;

            std::vector<float2> table(segments + 1);
            for (size_t i = 0; i <= segments; ++i) {
                const float angle = fullSweep * static_cast<float>(i) / static_cast<float>(segments);
                table[i] = {std::cos(angle), std::sin(angle)};
            }

            return tables.try_emplace(segments, std::move(table)).first->second;
        }

        std::unordered_map<size_t, std::vector<float2>> m_fullCircleTables;
        std::unordered_map<size_t, std::vector<float2>> m_quarterCircleTables;
    };

    inline thread_local UnitCircleTableCache unitCircleTableCache;
} // namespace p5cpp

namespace p5cpp
{
    inline static float4 colorToFloat4(color_t color)
    {
        static constexpr float inv255 = 1.0f / 255.0f;
        return float4 {
            static_cast<float>(red(color)) * inv255,
            static_cast<float>(green(color)) * inv255,
            static_cast<float>(blue(color)) * inv255,
            static_cast<float>(alpha(color)) * inv255,
        };
    }

    // A run of shaped glyphs making up one visual (post-wrap) line, shared by
    // text() (which draws it) and GraphicsComponent::layoutText() (which only
    // measures it) so the wrapping rules can't drift between the two.
    struct VisualLine
    {
        std::vector<ShapedGlyph> glyphs;
        float width = 0.0f;
    };

    static std::vector<VisualLine> shapeTextLines(const Font& font, std::string_view text, int textSizeInt, const RenderState& rs, float maxWidth)
    {
        const bool doWrap = (maxWidth > 0.0f) && (rs.textWrap != TextWrap::none);

        std::vector<VisualLine> lines;

        auto shapeParagraph = [&](std::string_view para) {
            if (para.empty()) {
                lines.push_back(VisualLine {});
                return;
            }

            std::vector<ShapedGlyph> shaped = font.shape(para, textSizeInt);

            if (!doWrap) {
                float w = 0.0f;
                for (const ShapedGlyph& g : shaped) w += g.xAdvance + rs.textLetterSpacing;
                lines.push_back(VisualLine {std::move(shaped), w});
                return;
            }

            if (rs.textWrap == TextWrap::character) {
                VisualLine current;
                for (const ShapedGlyph& g : shaped) {
                    const float adv = g.xAdvance + rs.textLetterSpacing;
                    if (current.width + adv > maxWidth && !current.glyphs.empty()) {
                        lines.push_back(std::move(current));
                        current = {};
                    }
                    current.glyphs.push_back(g);
                    current.width += adv;
                }
                if (!current.glyphs.empty()) lines.push_back(std::move(current));
                return;
            }

            // TextWrap::word — greedy fill at whitespace boundaries
            VisualLine current;
            for (size_t i = 0; i < shaped.size(); ++i) {
                const ShapedGlyph& g = shaped[i];
                const float adv = g.xAdvance + rs.textLetterSpacing;

                if (current.width + adv > maxWidth && !current.glyphs.empty()) {
                    int breakIdx = -1;
                    for (int j = static_cast<int>(current.glyphs.size()) - 1; j >= 0; --j) {
                        if (current.glyphs[j].isWhitespace) {
                            breakIdx = j;
                            break;
                        }
                    }

                    if (breakIdx >= 0) {
                        VisualLine completedLine;
                        for (size_t k = 0; k < static_cast<size_t>(breakIdx); ++k) {
                            completedLine.glyphs.push_back(current.glyphs[k]);
                            completedLine.width += current.glyphs[k].xAdvance + rs.textLetterSpacing;
                        }
                        lines.push_back(std::move(completedLine));

                        VisualLine newCurrent;
                        for (size_t k = static_cast<size_t>(breakIdx) + 1; k < current.glyphs.size(); ++k) {
                            newCurrent.glyphs.push_back(current.glyphs[k]);
                            newCurrent.width += current.glyphs[k].xAdvance + rs.textLetterSpacing;
                        }
                        current = std::move(newCurrent);
                    } else {
                        lines.push_back(std::move(current));
                        current = {};
                    }
                }

                current.glyphs.push_back(g);
                current.width += adv;
            }

            if (!current.glyphs.empty()) lines.push_back(std::move(current));
        };

        // Split on newlines and shape each paragraph
        size_t pos = 0;
        while (true) {
            const size_t nl = text.find('\n', pos);
            const size_t end = (nl == std::string_view::npos) ? text.size() : nl;
            shapeParagraph(text.substr(pos, end - pos));
            if (nl == std::string_view::npos) break;
            pos = nl + 1;
        }

        return lines;
    }
} // namespace p5cpp

namespace p5cpp
{
    inline static constexpr size_t MAX_VERTICES = 65536;
    inline static constexpr size_t MAX_INDICES = 65536 * 3;

    GraphicsComponent::GraphicsComponent(uint32_t width, uint32_t height)
        : m_drawPointCount(0),
          m_drawPointCapacity(0),
          m_curveVertexCount(0),
          m_canvas(width, height),
          m_renderStateStack(),
          m_defaultFont(loadFont(std::span {DejaVuSans_ttf, DejaVuSans_ttf_len})),
          m_renderer(NativeRenderer::create(MAX_VERTICES, MAX_INDICES))
    {
        m_defaultShader = createPrimitiveShader();
        m_textShader = createTextShader();

        const color_t white = rgba(255, 255, 255, 255);
        m_whiteTexture = loadTexture(1, 1, &white);
    }

    void GraphicsComponent::beginFrame()
    {
        pushCanvas(m_canvas.activeFramebuffer());
    }

    void GraphicsComponent::endFrame()
    {
        popCanvas();
        resolveMsaaToDefaultFramebuffer();
    }

    void GraphicsComponent::resizeDefaultCanvas(uint32_t width, uint32_t height)
    {
        m_canvas.resize(width, height);

        // beginFrame() pushes a *copy* of the active default canvas onto the stack (always
        // at index 0 - the outermost canvas). If a resize happens while that bracket is
        // still open (e.g. a Sketch calling setWindowSize() from within its own setup(),
        // after some earlier draw call already opened the frame), the stack entry would
        // otherwise keep referencing the old, now-orphaned framebuffer, so any further
        // drawing in the same setup()/draw() would silently land in a canvas nobody ever
        // presents to the screen again.
        swapActiveDefaultCanvas(m_canvas.activeFramebuffer());
    }

    void GraphicsComponent::smooth(uint32_t samples)
    {
        const uint32_t clamped = std::max<uint32_t>(samples, 1);
        if (m_canvas.samples() == clamped) return;

        // If we were already mid-frame in the (old) msaa target, carry forward what's
        // been drawn so far into the default framebuffer before swapping it out -
        // otherwise calling smooth()/noSmooth() partway through draw() would silently
        // drop whatever was drawn before the call. No-op if we weren't smoothing yet.
        resolveMsaaToDefaultFramebuffer();

        m_canvas.smooth(clamped);
        swapActiveDefaultCanvas(m_canvas.activeFramebuffer());

        // ...and paint that carried-forward content into the fresh msaa target so
        // drawing continues on top of it instead of a blank canvas.
        syncMsaaFromDefaultFramebuffer();
    }

    void GraphicsComponent::noSmooth()
    {
        if (!m_canvas.isEnabled()) return;

        resolveMsaaToDefaultFramebuffer();
        m_canvas.noSmooth();
        swapActiveDefaultCanvas(m_canvas.defaultFramebuffer());
    }

    void GraphicsComponent::swapActiveDefaultCanvas(const Framebuffer& newDefaultCanvas)
    {
        if (m_framebufferStack.empty()) return;

        m_framebufferStack.front() = newDefaultCanvas;
        if (m_framebufferStack.size() == 1) {
            m_renderer->begin(m_framebufferStack.back());
        }
    }

    void GraphicsComponent::resolveMsaaToDefaultFramebuffer()
    {
        // Always flush, even if there's nothing to resolve: callers (smooth() chief
        // among them) rely on this to push any pending-but-unflushed batches through to
        // GL before the framebuffer they were queued against might get swapped out from
        // under them - begin() on the new target would otherwise silently discard them
        // (it resets the writer/batch buffers without issuing their draw calls first).
        m_renderer->flush();
        if (!m_canvas.isEnabled()) return;

        m_canvas.resolveToDefault();

        if (not m_framebufferStack.empty()) {
            m_renderer->begin(m_framebufferStack.back());
        }
    }

    void GraphicsComponent::syncMsaaFromDefaultFramebuffer()
    {
        if (!m_canvas.isEnabled()) return;

        m_renderer->flush();
        m_renderer->end();

        m_canvas.syncFromDefault(*m_renderer, m_uniformCache, m_defaultShader);

        if (not m_framebufferStack.empty()) {
            m_renderer->begin(m_framebufferStack.back());
        }
    }

    void GraphicsComponent::blitDefaultCanvasToScreen(uint32_t screenWidth, uint32_t screenHeight)
    {
        const uint2 canvasSize = m_canvas.defaultFramebuffer().getSize();
        const GLuint fboId = m_canvas.defaultFramebuffer().getFramebufferId().value;

        glBindFramebuffer(GL_READ_FRAMEBUFFER, fboId);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(
            0, 0, static_cast<GLint>(canvasSize.x), static_cast<GLint>(canvasSize.y), 0, 0, static_cast<GLint>(screenWidth), static_cast<GLint>(screenHeight), GL_COLOR_BUFFER_BIT, (canvasSize.x == screenWidth && canvasSize.y == screenHeight) ? GL_NEAREST : GL_LINEAR
        );
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    void GraphicsComponent::pushCanvas(Framebuffer framebuffer)
    {
        if (m_recorder.isActive()) {
            assert(false && "pushCanvas() is not supported inside buildRenderGroup()");
            return;
        }

        m_renderer->flush();
        m_renderer->end();

        Framebuffer& current = m_framebufferStack.emplace_back(std::move(framebuffer));
        m_renderer->begin(current);

        m_renderStateStack.push();
        m_matrixStack.push();
    }

    void GraphicsComponent::popCanvas()
    {
        if (m_recorder.isActive()) {
            assert(false && "popCanvas() is not supported inside buildRenderGroup()");
            return;
        }

        m_renderer->flush();
        m_renderer->end();

        m_framebufferStack.pop_back();
        if (not m_framebufferStack.empty()) {
            Framebuffer& current = m_framebufferStack.back();
            m_renderer->begin(current);
        }

        m_renderStateStack.pop();
        m_matrixStack.pop();
    }

    uint2 GraphicsComponent::getCanvasSize()
    {
        if (m_framebufferStack.empty()) {
            return uint2::zero;
        }

        return m_framebufferStack.back().getSize();
    }

    Pixels GraphicsComponent::loadPixels()
    {
        if (m_framebufferStack.empty()) {
            return Pixels();
        }

        m_renderer->flush();

        // The multisample target can't be read directly (see detail::MultisampleFramebufferBackend);
        // resolve what's been drawn so far this frame into the default framebuffer first.
        if (m_canvas.isMsaaFramebuffer(m_framebufferStack.back())) {
            resolveMsaaToDefaultFramebuffer();
            const Framebuffer& resolved = m_canvas.defaultFramebuffer();
            const uint2 size = resolved.getSize();
            return Pixels(size.x, size.y, detail::flipRows(resolved.readPixels(), size.x, size.y));
        }

        const Framebuffer& framebuffer = m_framebufferStack.back();
        const uint2 size = framebuffer.getSize();
        return Pixels(size.x, size.y, detail::flipRows(framebuffer.readPixels(), size.x, size.y));
    }

    void GraphicsComponent::updatePixels(const Pixels& pixels)
    {
        if (m_framebufferStack.empty()) {
            return;
        }

        Framebuffer& framebuffer = m_framebufferStack.back();
        const uint2 size = framebuffer.getSize();
        assert(pixels.getWidth() == size.x && pixels.getHeight() == size.y);

        m_renderer->flush();

        if (m_canvas.isMsaaFramebuffer(framebuffer)) {
            m_canvas.defaultFramebuffer().writePixels(detail::flipRows(std::span<const color_t>(pixels.data(), pixels.size()), size.x, size.y));
            syncMsaaFromDefaultFramebuffer();
            return;
        }

        framebuffer.writePixels(detail::flipRows(std::span<const color_t>(pixels.data(), pixels.size()), size.x, size.y));
    }

    MatrixStack& GraphicsComponent::activeMatrixStack()
    {
        return m_recorder.activeMatrixStack(m_matrixStack);
    }

    RenderStateStack& GraphicsComponent::activeRenderStateStack()
    {
        return m_recorder.activeRenderStateStack(m_renderStateStack);
    }

    DrawBufferWriter& GraphicsComponent::beginDrawOp()
    {
        return m_recorder.beginDrawOp(*m_renderer);
    }

    void GraphicsComponent::endDrawOp(DrawBufferWriter& writer, const Shader& shader, const BlendMode& blendMode, const Texture& texture, std::span<const UniformSnapshot> uniforms)
    {
        m_recorder.endDrawOp(*m_renderer, writer, shader, blendMode, texture, uniforms);
    }

    void GraphicsComponent::pushState()
    {
        activeRenderStateStack().push();
    }

    void GraphicsComponent::popState()
    {
        activeRenderStateStack().pop();
    }

    void GraphicsComponent::pushMatrix()
    {
        activeMatrixStack().push();
    }

    void GraphicsComponent::popMatrix()
    {
        activeMatrixStack().pop();
    }

    void GraphicsComponent::push()
    {
        pushState();
        pushMatrix();
    }

    void GraphicsComponent::pop()
    {
        popMatrix();
        popState();
    }

    void GraphicsComponent::resetMatrix()
    {
        setMatrix(matrix4x4::identity);
    }

    matrix4x4& GraphicsComponent::peekMatrix()
    {
        return activeMatrixStack().peek();
    }

    void GraphicsComponent::applyMatrix(const matrix4x4& matrix)
    {
        matrix4x4& currentMatrix = activeMatrixStack().peek();
        currentMatrix = matrix * currentMatrix;
    }

    void GraphicsComponent::setMatrix(const matrix4x4& matrix)
    {
        activeMatrixStack().peek() = matrix;
    }

    void GraphicsComponent::translate(float x, float y)
    {
        applyMatrix(matrix4x4::translation(x, y));
    }

    void GraphicsComponent::scale(float x, float y)
    {
        applyMatrix(matrix4x4::scaling(x, y));
    }

    void GraphicsComponent::rotate(float radians)
    {
        applyMatrix(matrix4x4::rotation(radians));
    }

    void GraphicsComponent::fill(color_t color)
    {
        RenderState& currentState = peekRenderState();
        currentState.fillColor = color;
        currentState.isFillDisabled = false;
    }

    void GraphicsComponent::noFill()
    {
        RenderState& currentState = peekRenderState();
        currentState.isFillDisabled = true;
    }

    void GraphicsComponent::stroke(color_t color)
    {
        RenderState& currentState = peekRenderState();
        currentState.strokeColor = color;
        currentState.isStrokeDisabled = false;
    }

    void GraphicsComponent::noStroke()
    {
        RenderState& currentState = peekRenderState();
        currentState.isStrokeDisabled = true;
    }

    void GraphicsComponent::strokeWeight(float strokeWeight)
    {
        peekRenderState().strokeWeight = strokeWeight;
    }

    void GraphicsComponent::strokeCap(StrokeCap strokeCap)
    {
        peekRenderState().strokeCap = strokeCap;
    }

    void GraphicsComponent::strokeJoin(StrokeJoin strokeJoin)
    {
        peekRenderState().strokeJoin = strokeJoin;
    }

    void GraphicsComponent::miterLimit(float miterLimit)
    {
        peekRenderState().miterLimit = miterLimit;
    }

    void GraphicsComponent::roundJoinThreshold(float roundJoinThreshold)
    {
        peekRenderState().roundJoinThreshold = roundJoinThreshold;
    }

    void GraphicsComponent::tint(color_t color)
    {
        RenderState& currentState = peekRenderState();
        currentState.tintColor = color;
    }

    void GraphicsComponent::noTint()
    {
        RenderState& currentState = peekRenderState();
        currentState.tintColor = rgba(255, 255, 255, 255);
    }

    void GraphicsComponent::textureMode(TextureMode textureMode)
    {
        peekRenderState().textureMode = textureMode;
    }

    void GraphicsComponent::bezierDetail(uint32_t detail)
    {
        RenderState& currentState = peekRenderState();
        currentState.bezierDetail = detail;
        currentState.invBezierDetail = 1.0f / static_cast<float>(detail);
    }

    void GraphicsComponent::curveTightness(float tightness)
    {
        peekRenderState().curveTightness = tightness;
    }

    void GraphicsComponent::curveDetail(uint32_t detail)
    {
        RenderState& currentState = peekRenderState();
        currentState.curveDetail = detail;
        currentState.invCurveDetail = 1.0f / static_cast<float>(detail);
    }

    void GraphicsComponent::textFont(const Font& font)
    {
        peekRenderState().font = font;
    }

    void GraphicsComponent::noTextFont()
    {
        peekRenderState().font = std::nullopt;
    }

    void GraphicsComponent::textSize(float size)
    {
        peekRenderState().textSize = size;
    }

    void GraphicsComponent::textLetterSpacing(float letterSpacing)
    {
        peekRenderState().textLetterSpacing = letterSpacing;
    }

    void GraphicsComponent::textLineSpacing(float lineSpacing)
    {
        peekRenderState().textLineSpacing = lineSpacing;
    }

    void GraphicsComponent::textAlign(TextAlign align)
    {
        peekRenderState().textAlign = align;
    }

    void GraphicsComponent::textWrap(TextWrap wrap)
    {
        peekRenderState().textWrap = wrap;
    }

    void GraphicsComponent::textToPointsDetail(uint32_t detail)
    {
        peekRenderState().textToPointsDetail = detail;
    }

    void GraphicsComponent::textToPointsSpacing(float spacing)
    {
        peekRenderState().textToPointsSpacing = spacing;
    }

    void GraphicsComponent::shader(const Shader& shader)
    {
        peekRenderState().shader = shader;
    }

    void GraphicsComponent::noShader()
    {
        peekRenderState().shader = std::nullopt;
    }

    void GraphicsComponent::blendMode(BlendMode blendMode)
    {
        peekRenderState().blendMode = blendMode;
    }

    void GraphicsComponent::setUniform(const std::string& name, const UniformVariable& variable)
    {
        const RenderState& renderState = peekRenderState();
        if (renderState.shader.has_value()) {
            m_uniformCache.setUniform(renderState.shader.value(), name, variable);
        }
    }

    void GraphicsComponent::setUniform(const Shader& shader, const std::string& name, const UniformVariable& variable)
    {
        m_uniformCache.setUniform(shader, name, variable);
    }

    RenderState& GraphicsComponent::peekRenderState()
    {
        return activeRenderStateStack().peek();
    }

    void GraphicsComponent::background(color_t color)
    {
        if (m_recorder.isActive()) {
            assert(false && "background() is not supported inside buildRenderGroup()");
            return;
        }

        const uint2 canvasSize = getCanvasSize();
        const float w = static_cast<float>(canvasSize.x);
        const float h = static_cast<float>(canvasSize.y);
        const float4 col = colorToFloat4(color);

        DrawBufferWriter& writer = m_renderer->getDrawScope();
        const uint32_t base = writer.getRelativeCursor();

        writer.pushVertex({0.0f, 0.0f}, {0.0f, 0.0f}, col);
        writer.pushVertex({w, 0.0f}, {1.0f, 0.0f}, col);
        writer.pushVertex({w, h}, {1.0f, 1.0f}, col);
        writer.pushVertex({0.0f, h}, {0.0f, 1.0f}, col);
        writer.pushTriangle(base + 0, base + 1, base + 2);
        writer.pushTriangle(base + 0, base + 2, base + 3);

        m_renderer->submit(writer, m_uniformCache.getUniforms(m_defaultShader), m_defaultShader, BlendMode::alpha, m_whiteTexture);
    }

    void GraphicsComponent::rect(float left, float top, float w, float h)
    {
        const RenderState& rs = peekRenderState();
        const matrix4x4& mtx = peekMatrix();

        const float2 p0 = transformPoint(mtx, left, top);
        const float2 p1 = transformPoint(mtx, left + w, top);
        const float2 p2 = transformPoint(mtx, left + w, top + h);
        const float2 p3 = transformPoint(mtx, left, top + h);

        const float2 positions[4] = {p0, p1, p2, p3};
        const float2 uvs[4] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

        if (not rs.isFillDisabled) {
            const color_t fillColors[4] = {rs.fillColor, rs.fillColor, rs.fillColor, rs.fillColor};
            submitFill(PathPoints {4, positions, uvs, fillColors}, ShapeType::quads, m_whiteTexture);
        }

        if (not rs.isStrokeDisabled) {
            const color_t strokeColors[4] = {rs.strokeColor, rs.strokeColor, rs.strokeColor, rs.strokeColor};
            submitStroke(PathPoints {4, positions, uvs, strokeColors}, ShapeType::polygon, true);
        }
    }

    void GraphicsComponent::rect(float left, float top, float w, float h, BorderRadius borderRadius)
    {
        const RenderState& rs = peekRenderState();
        const matrix4x4& mtx = peekMatrix();

        m_roundedRectPositions.clear();

        const auto addCornerArc = [&](float cx, float cy, float rx, float ry, float startAngle) {
            const float maxR = std::max(rx, ry);
            const size_t segments = (maxR > 0.0f) ? computeCircleSegmentCount(HALF_PI, maxR) : 1;
            const std::vector<float2>& quarterCircle = unitCircleTableCache.getQuarterCircle(segments);
            const float cosStart = std::cos(startAngle);
            const float sinStart = std::sin(startAngle);
            for (size_t i = 0; i <= segments; ++i) {
                const float cosQ = quarterCircle[i].x;
                const float sinQ = quarterCircle[i].y;
                const float cosA = cosStart * cosQ - sinStart * sinQ;
                const float sinA = sinStart * cosQ + cosStart * sinQ;
                m_roundedRectPositions.push_back(transformPoint(mtx, cx + cosA * rx, cy + sinA * ry));
            }
        };

        addCornerArc(left + borderRadius.topLeft.x, top + borderRadius.topLeft.y, borderRadius.topLeft.x, borderRadius.topLeft.y, PI);
        addCornerArc(left + w - borderRadius.topRight.x, top + borderRadius.topRight.y, borderRadius.topRight.x, borderRadius.topRight.y, 3.0f * HALF_PI);
        addCornerArc(left + w - borderRadius.bottomRight.x, top + h - borderRadius.bottomRight.y, borderRadius.bottomRight.x, borderRadius.bottomRight.y, 0.0f);
        addCornerArc(left + borderRadius.bottomLeft.x, top + h - borderRadius.bottomLeft.y, borderRadius.bottomLeft.x, borderRadius.bottomLeft.y, HALF_PI);

        const size_t n = m_roundedRectPositions.size();
        m_roundedRectUVs.assign(n, float2::zero);

        if (!rs.isFillDisabled) {
            m_roundedRectFillColors.assign(n, rs.fillColor);
            submitFill(PathPoints {n, m_roundedRectPositions, m_roundedRectUVs, m_roundedRectFillColors}, ShapeType::polygon, m_whiteTexture);
        }

        if (!rs.isStrokeDisabled) {
            m_roundedRectStrokeColors.assign(n, rs.strokeColor);
            submitStroke(PathPoints {n, m_roundedRectPositions, m_roundedRectUVs, m_roundedRectStrokeColors}, ShapeType::polygon, true);
        }
    }

    void GraphicsComponent::ellipse(float cx, float cy, float rx, float ry)
    {
        const RenderState& rs = peekRenderState();
        const float maxRadius = std::max(rx, ry);
        const size_t segments = computeCircleSegmentCount(TWO_PI, maxRadius);
        const matrix4x4& mtx = peekMatrix();
        const std::vector<float2>& unitCircle = unitCircleTableCache.getFullCircle(segments);

        // Build center + (segments+1) perimeter points for a closed fan.
        // The last perimeter point == the first (closing duplicate) so the fan triangle
        // (center, last, first_perimeter+1) closes the circle.
        const size_t fanCount = segments + 2;
        m_ellipseFanPositions.resize(fanCount);
        m_ellipseFanUVs.resize(fanCount);

        m_ellipseFanPositions[0] = transformPoint(mtx, cx, cy);
        m_ellipseFanUVs[0] = {0.5f, 0.5f};

        for (size_t i = 0; i <= segments; ++i) {
            const float cosA = unitCircle[i].x;
            const float sinA = unitCircle[i].y;
            m_ellipseFanPositions[1 + i] = transformPoint(mtx, cx + cosA * rx, cy + sinA * ry);
            m_ellipseFanUVs[1 + i] = {0.5f + 0.5f * cosA, 0.5f + 0.5f * sinA};
        }

        if (not rs.isFillDisabled) {
            m_ellipseFillColors.assign(fanCount, rs.fillColor);
            submitFill(PathPoints {fanCount, m_ellipseFanPositions, m_ellipseFanUVs, m_ellipseFillColors}, ShapeType::triangleFan, m_whiteTexture);
        }

        if (not rs.isStrokeDisabled) {
            // Stroke uses exactly `segments` perimeter points (no center, no closing duplicate).
            m_ellipseStrokeUVs.assign(segments, float2::zero);
            m_ellipseStrokeColors.assign(segments, rs.strokeColor);
            submitStroke(
                PathPoints {segments, {m_ellipseFanPositions.data() + 1, segments}, m_ellipseStrokeUVs, m_ellipseStrokeColors},
                ShapeType::lineLoop,
                true
            );
        }
    }

    void GraphicsComponent::triangle(float x1, float y1, float x2, float y2, float x3, float y3)
    {
        const RenderState& rs = peekRenderState();
        const matrix4x4& mtx = peekMatrix();

        const float2 positions[3] = {
            transformPoint(mtx, x1, y1),
            transformPoint(mtx, x2, y2),
            transformPoint(mtx, x3, y3),
        };
        const float2 uvs[3] = {float2::zero, float2::zero, float2::zero};

        if (!rs.isFillDisabled) {
            const color_t fillColors[3] = {rs.fillColor, rs.fillColor, rs.fillColor};
            submitFill(PathPoints {3, positions, uvs, fillColors}, ShapeType::triangles, m_whiteTexture);
        }

        if (!rs.isStrokeDisabled) {
            const color_t strokeColors[3] = {rs.strokeColor, rs.strokeColor, rs.strokeColor};
            submitStroke(PathPoints {3, positions, uvs, strokeColors}, ShapeType::polygon, true);
        }
    }

    void GraphicsComponent::point(float px, float py)
    {
        const RenderState& rs = peekRenderState();
        if (rs.isStrokeDisabled) return;

        const float halfSize = rs.strokeWeight * 0.5f;
        const matrix4x4& mtx = peekMatrix();
        const float2 center = transformPoint(mtx, px, py);

        if (rs.strokeCap.start == StrokeCapStyle::round) {
            const size_t segments = computeCircleSegmentCount(TWO_PI, halfSize);
            const std::vector<float2>& unitCircle = unitCircleTableCache.getFullCircle(segments);
            const size_t fanCount = segments + 2;
            m_pointFanPositions.resize(fanCount);
            m_pointFanUVs.assign(fanCount, float2::zero);
            m_pointFanColors.assign(fanCount, rs.strokeColor);

            m_pointFanPositions[0] = center;
            for (size_t i = 0; i <= segments; ++i) {
                m_pointFanPositions[1 + i] = center + unitCircle[i] * halfSize;
            }

            submitFill(PathPoints {fanCount, m_pointFanPositions, m_pointFanUVs, m_pointFanColors}, ShapeType::triangleFan, m_whiteTexture);
        } else {
            const float2 positions[4] = {
                {center.x - halfSize, center.y - halfSize},
                {center.x + halfSize, center.y - halfSize},
                {center.x + halfSize, center.y + halfSize},
                {center.x - halfSize, center.y + halfSize},
            };
            const float2 uvs[4] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
            const color_t colors[4] = {rs.strokeColor, rs.strokeColor, rs.strokeColor, rs.strokeColor};
            submitFill(PathPoints {4, positions, uvs, colors}, ShapeType::quads, m_whiteTexture);
        }
    }

    void GraphicsComponent::line(float x1, float y1, float x2, float y2)
    {
        const RenderState& rs = peekRenderState();
        if (rs.isStrokeDisabled) return;

        const matrix4x4& mtx = peekMatrix();
        const float2 positions[2] = {transformPoint(mtx, x1, y1), transformPoint(mtx, x2, y2)};
        const float2 uvs[2] = {float2::zero, float2::zero};
        const color_t colors[2] = {rs.strokeColor, rs.strokeColor};

        submitStroke(PathPoints {2, positions, uvs, colors}, ShapeType::lines, false);
    }

    void GraphicsComponent::arc(float cx, float cy, float w, float h, float startAngle, float sweepAngle, ArcMode arcMode)
    {
        const RenderState& rs = peekRenderState();
        const float rx = w * 0.5f;
        const float ry = h * 0.5f;
        const float maxRadius = std::max(rx, ry);
        const size_t segments = computeCircleSegmentCount(std::abs(sweepAngle), maxRadius);
        const matrix4x4& mtx = peekMatrix();

        // Arc perimeter points
        m_arcPositions.resize(segments + 1);
        for (size_t i = 0; i <= segments; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(segments);
            const float angle = startAngle + sweepAngle * t;
            m_arcPositions[i] = transformPoint(mtx, cx + std::cos(angle) * rx, cy + std::sin(angle) * ry);
        }

        const float2 centerPos = transformPoint(mtx, cx, cy);

        if (!rs.isFillDisabled) {
            m_arcFillPositions.clear();
            if (arcMode == ArcMode::pie) {
                m_arcFillPositions.reserve(segments + 3);
                m_arcFillPositions.push_back(centerPos);
                m_arcFillPositions.insert(m_arcFillPositions.end(), m_arcPositions.begin(), m_arcPositions.end());
            } else {
                m_arcFillPositions = m_arcPositions;
            }
            m_arcFillUVs.assign(m_arcFillPositions.size(), float2::zero);
            m_arcFillColors.assign(m_arcFillPositions.size(), rs.fillColor);
            submitFill(PathPoints {m_arcFillPositions.size(), m_arcFillPositions, m_arcFillUVs, m_arcFillColors}, ShapeType::polygon, m_whiteTexture);
        }

        if (!rs.isStrokeDisabled) {
            m_arcStrokeUVs.assign(m_arcPositions.size(), float2::zero);
            m_arcStrokeColors.assign(m_arcPositions.size(), rs.strokeColor);
            const PathPoints arcPts {m_arcPositions.size(), m_arcPositions, m_arcStrokeUVs, m_arcStrokeColors};

            if (arcMode == ArcMode::open) {
                submitStroke(arcPts, ShapeType::lineStrip, false);
            } else if (arcMode == ArcMode::chord) {
                submitStroke(arcPts, ShapeType::lineLoop, true);
            } else { // pie
                m_arcPiePositions.clear();
                m_arcPiePositions.reserve(m_arcPositions.size() + 2);
                m_arcPiePositions.push_back(centerPos);
                m_arcPiePositions.insert(m_arcPiePositions.end(), m_arcPositions.begin(), m_arcPositions.end());
                m_arcPieUVs.assign(m_arcPiePositions.size(), float2::zero);
                m_arcPieColors.assign(m_arcPiePositions.size(), rs.strokeColor);
                submitStroke(PathPoints {m_arcPiePositions.size(), m_arcPiePositions, m_arcPieUVs, m_arcPieColors}, ShapeType::polygon, true);
            }
        }
    }

    void GraphicsComponent::bezier(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
    {
        const RenderState& rs = peekRenderState();
        if (rs.isStrokeDisabled) return;

        const size_t detail = rs.bezierDetail;
        const float invDetail = rs.invBezierDetail;
        const matrix4x4& mtx = peekMatrix();

        const size_t count = detail + 1;
        m_curvePositions.resize(count);
        m_curveUVs.assign(count, float2::zero);
        m_curveColors.assign(count, rs.strokeColor);

        for (size_t j = 0; j <= detail; ++j) {
            const float t = static_cast<float>(j) * invDetail;
            const float mt = 1.0f - t;
            const float mt2 = mt * mt;
            const float mt3 = mt2 * mt;
            const float t2 = t * t;
            const float t3 = t2 * t;
            const float bx = mt3 * x1 + 3.0f * mt2 * t * x2 + 3.0f * mt * t2 * x3 + t3 * x4;
            const float by = mt3 * y1 + 3.0f * mt2 * t * y2 + 3.0f * mt * t2 * y3 + t3 * y4;
            m_curvePositions[j] = transformPoint(mtx, bx, by);
        }

        submitStroke(PathPoints {count, m_curvePositions, m_curveUVs, m_curveColors}, ShapeType::lineStrip, false);
    }

    void GraphicsComponent::curve(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
    {
        const RenderState& rs = peekRenderState();
        if (rs.isStrokeDisabled) return;

        const float alpha = (1.0f - rs.curveTightness) * 0.5f;
        const size_t detail = rs.curveDetail;
        const float invDetail = rs.invCurveDetail;
        const matrix4x4& mtx = peekMatrix();

        const size_t count = detail + 1;
        m_curvePositions.resize(count);
        m_curveUVs.assign(count, float2::zero);
        m_curveColors.assign(count, rs.strokeColor);

        for (size_t j = 0; j <= detail; ++j) {
            const float t = static_cast<float>(j) * invDetail;
            const float t2 = t * t;
            const float t3 = t2 * t;
            const float bx = alpha * ((-x1 + 3.0f * x2 - 3.0f * x3 + x4) * t3 + (2.0f * x1 - 5.0f * x2 + 4.0f * x3 - x4) * t2 + (-x1 + x3) * t) + x2;
            const float by = alpha * ((-y1 + 3.0f * y2 - 3.0f * y3 + y4) * t3 + (2.0f * y1 - 5.0f * y2 + 4.0f * y3 - y4) * t2 + (-y1 + y3) * t) + y2;
            m_curvePositions[j] = transformPoint(mtx, bx, by);
        }

        submitStroke(PathPoints {count, m_curvePositions, m_curveUVs, m_curveColors}, ShapeType::lineStrip, false);
    }

    void GraphicsComponent::image(const Texture& texture, float left, float top, float w, float h)
    {
        const RenderState& rs = peekRenderState();
        const matrix4x4& mtx = peekMatrix();
        const float4 tint = colorToFloat4(rs.tintColor);

        DrawBufferWriter& writer = beginDrawOp();
        const uint32_t base = writer.getRelativeCursor();

        // Texture v-origin is bottom (GL row order, see Texture::upload()), so the
        // top edge of the destination rect samples v=1 and the bottom edge samples v=0.
        writer.pushVertex(transformPoint(mtx, left, top), {0.0f, 1.0f}, tint);
        writer.pushVertex(transformPoint(mtx, left + w, top), {1.0f, 1.0f}, tint);
        writer.pushVertex(transformPoint(mtx, left + w, top + h), {1.0f, 0.0f}, tint);
        writer.pushVertex(transformPoint(mtx, left, top + h), {0.0f, 0.0f}, tint);
        writer.pushTriangle(base + 0, base + 1, base + 2);
        writer.pushTriangle(base + 0, base + 2, base + 3);

        const Shader shaderToUse = getShader(rs);
        endDrawOp(writer, shaderToUse, rs.blendMode, texture, m_uniformCache.getUniforms(shaderToUse));
    }

    void GraphicsComponent::image(const Texture& texture, float left, float top, float w, float h, float sx, float sy, float sw, float sh)
    {
        const RenderState& rs = peekRenderState();
        const matrix4x4& mtx = peekMatrix();
        const float4 tint = colorToFloat4(rs.tintColor);

        // Normalize sx/sy/sw/sh (top-left origin, see TextureMode) to a 0..1 UV rect
        // in that same top-left-origin space first...
        float nsx = sx;
        float nsy = sy;
        float nsw = sw;
        float nsh = sh;
        if (rs.textureMode == TextureMode::image) {
            const uint2 texSize = texture.getSize();
            const float invW = texSize.x > 0 ? 1.0f / static_cast<float>(texSize.x) : 0.0f;
            const float invH = texSize.y > 0 ? 1.0f / static_cast<float>(texSize.y) : 0.0f;
            nsx = sx * invW;
            nsy = sy * invH;
            nsw = sw * invW;
            nsh = sh * invH;
        }

        // ...then flip to the texture's bottom-origin GL v (see Texture::upload()):
        // the source rect's top edge (nsy) samples the higher v, its bottom edge
        // (nsy + nsh) samples the lower v — mirroring the plain image() overload above.
        const float u0 = nsx;
        const float u1 = nsx + nsw;
        const float v0 = 1.0f - nsy;
        const float v1 = 1.0f - (nsy + nsh);

        DrawBufferWriter& writer = beginDrawOp();
        const uint32_t base = writer.getRelativeCursor();

        writer.pushVertex(transformPoint(mtx, left, top), {u0, v0}, tint);
        writer.pushVertex(transformPoint(mtx, left + w, top), {u1, v0}, tint);
        writer.pushVertex(transformPoint(mtx, left + w, top + h), {u1, v1}, tint);
        writer.pushVertex(transformPoint(mtx, left, top + h), {u0, v1}, tint);
        writer.pushTriangle(base + 0, base + 1, base + 2);
        writer.pushTriangle(base + 0, base + 2, base + 3);

        const Shader shaderToUse = getShader(rs);
        endDrawOp(writer, shaderToUse, rs.blendMode, texture, m_uniformCache.getUniforms(shaderToUse));
    }

    void GraphicsComponent::text(std::string_view text, float x, float y)
    {
        this->text(text, x, y, -1.0f);
    }

    void GraphicsComponent::text(std::string_view text, float x, float y, float maxWidth)
    {
        const RenderState& rs = peekRenderState();
        const Font& font = rs.font.value_or(m_defaultFont);
        if (rs.isFillDisabled) return;

        const int textSizeInt = static_cast<int>(rs.textSize);
        if (textSizeInt <= 0) return;

        const FontMetrics* metrics = font.getMetrics(textSizeInt);
        if (!metrics) return;

        const float lineH = metrics->lineHeight * rs.textLineSpacing;
        std::vector<VisualLine> lines = shapeTextLines(font, text, textSizeInt, rs, maxWidth);
        if (lines.empty()) return;

        // Vertical alignment
        const float totalH = lineH * static_cast<float>(lines.size());

        float penY0 = y;
        switch (rs.textAlign.vertical) {
            case VerticalTextAlign::top:
                penY0 = y + metrics->ascender;
                break;
            case VerticalTextAlign::center:
                penY0 = y - totalH * 0.5f + metrics->ascender;
                break;
            case VerticalTextAlign::bottom:
                penY0 = y - totalH + metrics->ascender;
                break;
            case VerticalTextAlign::baseline:
            default:
                penY0 = y;
                break;
        }

        const matrix4x4& mtx = peekMatrix();
        const float4 fillColor = colorToFloat4(rs.fillColor);

        for (size_t li = 0; li < lines.size(); ++li) {
            const VisualLine& line = lines[li];
            const float baseY = penY0 + static_cast<float>(li) * lineH;

            float startX = x;
            switch (rs.textAlign.horizontal) {
                case HorizontalTextAlign::left:
                    startX = x;
                    break;
                case HorizontalTextAlign::center:
                    startX = x - line.width * 0.5f;
                    break;
                case HorizontalTextAlign::right:
                    startX = x - line.width;
                    break;
            }

            float penX = startX;
            float penY = baseY;

            for (const ShapedGlyph& sg : line.glyphs) {
                if (!sg.isWhitespace && sg.size.x > 0 && sg.size.y > 0) {
                    const Texture* atlasTexture = font.getGlyphAtlasTexture(sg.glyphAtlasIndex);
                    if (atlasTexture) {
                        const float gLeft = penX + sg.xOffset + static_cast<float>(sg.bearing.x);
                        const float gTop = penY - sg.yOffset - static_cast<float>(sg.bearing.y);
                        const float gW = static_cast<float>(sg.size.x);
                        const float gH = static_cast<float>(sg.size.y);

                        const float u0 = sg.uvRect.left;
                        const float v0 = sg.uvRect.top;
                        const float u1 = sg.uvRect.left + sg.uvRect.width;
                        const float v1 = sg.uvRect.top + sg.uvRect.height;

                        DrawBufferWriter& writer = beginDrawOp();
                        const uint32_t base = writer.getRelativeCursor();

                        writer.pushVertex(transformPoint(mtx, gLeft, gTop), {u0, v0}, fillColor);
                        writer.pushVertex(transformPoint(mtx, gLeft + gW, gTop), {u1, v0}, fillColor);
                        writer.pushVertex(transformPoint(mtx, gLeft + gW, gTop + gH), {u1, v1}, fillColor);
                        writer.pushVertex(transformPoint(mtx, gLeft, gTop + gH), {u0, v1}, fillColor);
                        writer.pushTriangle(base + 0, base + 1, base + 2);
                        writer.pushTriangle(base + 0, base + 2, base + 3);

                        endDrawOp(writer, m_textShader, rs.blendMode, *atlasTexture, m_uniformCache.getUniforms(m_textShader));
                    }
                }

                penX += sg.xAdvance + rs.textLetterSpacing;
                penY += sg.yAdvance;
            }
        }
    }

    TextLayout GraphicsComponent::layoutText(std::string_view text, float x, float y)
    {
        return this->layoutText(text, x, y, -1.0f);
    }

    TextLayout GraphicsComponent::layoutText(std::string_view text, float x, float y, float maxWidth)
    {
        TextLayout layout;

        const RenderState& rs = peekRenderState();
        const Font& font = rs.font.value_or(m_defaultFont);

        const int textSizeInt = static_cast<int>(rs.textSize);
        if (textSizeInt <= 0) return layout;

        const FontMetrics* metrics = font.getMetrics(textSizeInt);
        if (!metrics) return layout;

        const float lineH = metrics->lineHeight * rs.textLineSpacing;
        std::vector<VisualLine> lines = shapeTextLines(font, text, textSizeInt, rs, maxWidth);
        if (lines.empty()) return layout;

        const float totalH = lineH * static_cast<float>(lines.size());

        float penY0 = y;
        switch (rs.textAlign.vertical) {
            case VerticalTextAlign::top:
                penY0 = y + metrics->ascender;
                break;
            case VerticalTextAlign::center:
                penY0 = y - totalH * 0.5f + metrics->ascender;
                break;
            case VerticalTextAlign::bottom:
                penY0 = y - totalH + metrics->ascender;
                break;
            case VerticalTextAlign::baseline:
            default:
                penY0 = y;
                break;
        }

        layout.lines.reserve(lines.size());
        layout.ascender = metrics->ascender;
        layout.descender = metrics->descender;
        layout.lineHeight = lineH;
        layout.height = totalH;

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();

        for (size_t li = 0; li < lines.size(); ++li) {
            const VisualLine& line = lines[li];
            const float baseY = penY0 + static_cast<float>(li) * lineH;

            float startX = x;
            switch (rs.textAlign.horizontal) {
                case HorizontalTextAlign::left:
                    startX = x;
                    break;
                case HorizontalTextAlign::center:
                    startX = x - line.width * 0.5f;
                    break;
                case HorizontalTextAlign::right:
                    startX = x - line.width;
                    break;
            }

            layout.lines.push_back(TextLineLayout {.width = line.width, .x = startX, .baselineY = baseY});
            layout.width = std::max(layout.width, line.width);
            minX = std::min(minX, startX);
            maxX = std::max(maxX, startX + line.width);
        }

        layout.bounds = float_rect {minX, penY0 - metrics->ascender, maxX - minX, totalH};

        return layout;
    }

    std::vector<TextContour> GraphicsComponent::textToPoints(std::string_view text, float x, float y)
    {
        return this->textToPoints(text, x, y, -1.0f);
    }

    std::vector<TextContour> GraphicsComponent::textToPoints(std::string_view text, float x, float y, float maxWidth)
    {
        const RenderState& rs = peekRenderState();
        const Font& font = rs.font.value_or(m_defaultFont);

        const int textSizeInt = static_cast<int>(rs.textSize);
        if (textSizeInt <= 0) return {};

        std::vector<TextContour> contours = font.textToPoints(text, x, y, textSizeInt, static_cast<int>(rs.textToPointsDetail), rs.textToPointsSpacing, maxWidth, rs.textWrap);

        const matrix4x4& mtx = peekMatrix();
        for (TextContour& contour : contours) {
            for (float2& p : contour) {
                p = transformPoint(mtx, p.x, p.y);
            }
        }

        return contours;
    }

    void GraphicsComponent::beginShape()
    {
        m_drawPointCount = 0;
        m_curveVertexCount = 0;
    }

    void GraphicsComponent::endShape(ShapeType type, bool close)
    {
        endShapeImpl(type, close, peekRenderState());
    }

    void GraphicsComponent::vertex(float x, float y, float u, float v)
    {
        if (m_drawPointCount >= m_drawPointCapacity) {
            const size_t newCapacity = std::max(m_drawPointCount * 2uz, 4uz);

            std::unique_ptr<float2[]> newPositions = std::make_unique<float2[]>(newCapacity);
            std::unique_ptr<float2[]> newTexCoords = std::make_unique<float2[]>(newCapacity);
            std::unique_ptr<color_t[]> newFillColors = std::make_unique<color_t[]>(newCapacity);
            std::unique_ptr<color_t[]> newStrokeColors = std::make_unique<color_t[]>(newCapacity);

            if (m_drawPointCount > 0) {
                std::copy(m_drawPointPositions.get(), m_drawPointPositions.get() + m_drawPointCount, newPositions.get());
                std::copy(m_drawPointTexCoords.get(), m_drawPointTexCoords.get() + m_drawPointCount, newTexCoords.get());
                std::copy(m_drawPointFillColors.get(), m_drawPointFillColors.get() + m_drawPointCount, newFillColors.get());
                std::copy(m_drawPointStrokeColors.get(), m_drawPointStrokeColors.get() + m_drawPointCount, newStrokeColors.get());
            }

            m_drawPointPositions = std::move(newPositions);
            m_drawPointTexCoords = std::move(newTexCoords);
            m_drawPointFillColors = std::move(newFillColors);
            m_drawPointStrokeColors = std::move(newStrokeColors);
            m_drawPointCapacity = newCapacity;
        }

        const RenderState& renderState = peekRenderState();
        const matrix4x4& matrix = peekMatrix();
        const float2 transformedPosition = transformPoint(matrix, x, y);

        m_drawPointPositions[m_drawPointCount] = transformedPosition;
        m_drawPointTexCoords[m_drawPointCount] = float2 {u, v};
        m_drawPointFillColors[m_drawPointCount] = renderState.fillColor;
        m_drawPointStrokeColors[m_drawPointCount] = renderState.strokeColor;
        ++m_drawPointCount;

        m_curveVertexCount = 0; // Reset curve vertex count when a regular vertex is added
    }

    void GraphicsComponent::curveVertex(float x, float y)
    {
        m_curveVertexPositions[m_curveVertexCount] = float2 {x, y};
        m_curveVertexCount++;

        if (m_curveVertexCount == 4) {
            const RenderState& renderState = peekRenderState();
            const float alpha = (1.0f - renderState.curveTightness) * 0.5f;

            auto [x1, y1] = m_curveVertexPositions[0];
            auto [x2, y2] = m_curveVertexPositions[1];
            auto [x3, y3] = m_curveVertexPositions[2];
            auto [x4, y4] = m_curveVertexPositions[3];

            for (size_t j = 0; j <= renderState.curveDetail; ++j) {
                float t = static_cast<float>(j) * renderState.invCurveDetail;
                float t2 = t * t;
                float t3 = t2 * t;

                float bx = alpha * ((-x1 + 3 * x2 - 3 * x3 + x4) * t3 + (2 * x1 - 5 * x2 + 4 * x3 - x4) * t2 + (-x1 + x3) * t) + x2;
                float by = alpha * ((-y1 + 3 * y2 - 3 * y3 + y4) * t3 + (2 * y1 - 5 * y2 + 4 * y3 - y4) * t2 + (-y1 + y3) * t) + y2;

                vertex(bx, by, 0.0f, 0.0f);
            }

            m_curveVertexCount = 0;
        }
    }

    void GraphicsComponent::endShapeImpl(ShapeType type, bool close, const RenderState& renderState)
    {
        if (m_drawPointCount == 0) return;

        const PathPoints fillPts {
            m_drawPointCount,
            {m_drawPointPositions.get(), m_drawPointCount},
            {m_drawPointTexCoords.get(), m_drawPointCount},
            {m_drawPointFillColors.get(), m_drawPointCount},
        };
        const PathPoints strokePts {
            m_drawPointCount,
            {m_drawPointPositions.get(), m_drawPointCount},
            {m_drawPointTexCoords.get(), m_drawPointCount},
            {m_drawPointStrokeColors.get(), m_drawPointCount},
        };

        if (not renderState.isFillDisabled) {
            submitFill(fillPts, type, m_whiteTexture);
        }

        if (not renderState.isStrokeDisabled) {
            submitStroke(strokePts, type, close);
        }

        m_drawPointCount = 0;
    }

    RenderGroup GraphicsComponent::buildRenderGroup(const std::function<void()>& buildFn)
    {
        return m_recorder.build(buildFn);
    }

    void GraphicsComponent::drawRenderGroup(const RenderGroup& group)
    {
        const std::shared_ptr<const RenderGroupImpl>& impl = group.getImpl();
        if (!impl) return;

        const matrix4x4& mtx = peekMatrix();

        for (const RecordedOp& op : impl->ops) {
            DrawBufferWriter& writer = beginDrawOp();
            const uint32_t base = writer.getRelativeCursor();

            for (const RecordedVertex& v : op.vertices) {
                writer.pushVertex(transformPoint(mtx, v.position.x, v.position.y), v.texcoord, v.color);
            }

            for (size_t i = 0; i + 2 < op.indices.size(); i += 3) {
                writer.pushTriangle(base + op.indices[i], base + op.indices[i + 1], base + op.indices[i + 2]);
            }

            endDrawOp(writer, op.shader, op.blendMode, op.texture, op.uniforms);
        }
    }

    Shader GraphicsComponent::getShader(const RenderState& renderState)
    {
        if (renderState.shader.has_value()) {
            return renderState.shader.value();
        }

        return m_defaultShader;
    }

    void GraphicsComponent::submitFill(const PathPoints& pts, ShapeType type, const Texture& texture)
    {
        const RenderState& rs = peekRenderState();
        DrawBufferWriter& writer = beginDrawOp();

        switch (type) {
            case ShapeType::triangles: tesselate_triangles(writer, pts); break;
            case ShapeType::triangleStrip: tesselate_triangle_strip(writer, pts); break;
            case ShapeType::triangleFan: tesselate_triangle_fan(writer, pts); break;
            case ShapeType::quads: tesselate_quads(writer, pts); break;
            case ShapeType::quadStrip: tesselate_quad_strip(writer, pts); break;
            case ShapeType::polygon: tesselate_polygon(writer, pts); break;
            default: throw std::invalid_argument("Unsupported shape type for fill"); break;
        }

        const Shader shaderToUse = getShader(rs);
        endDrawOp(writer, shaderToUse, rs.blendMode, texture, m_uniformCache.getUniforms(shaderToUse));
    }

    void GraphicsComponent::submitStroke(const PathPoints& pts, ShapeType type, bool close)
    {
        const RenderState& rs = peekRenderState();
        DrawBufferWriter& writer = beginDrawOp();

        switch (type) {
            case ShapeType::lines: stroke_lines(writer, pts, rs.strokeWeight, rs.strokeCap, rs.miterLimit, rs.roundJoinThreshold, computeCircleSegmentCount); break;
            case ShapeType::lineStrip: stroke_line_strip(writer, pts, rs.strokeWeight, rs.strokeCap, rs.strokeJoin, rs.miterLimit, rs.roundJoinThreshold, computeCircleSegmentCount); break;
            case ShapeType::lineLoop: stroke_line_loop(writer, pts, rs.strokeWeight, rs.strokeJoin, rs.miterLimit, rs.roundJoinThreshold, computeCircleSegmentCount); break;
            case ShapeType::triangles: stroke_triangles(writer, pts, rs.strokeWeight, rs.strokeCap, rs.strokeJoin, rs.miterLimit, rs.roundJoinThreshold, computeCircleSegmentCount); break;
            case ShapeType::triangleStrip: stroke_triangle_strip(writer, pts, rs.strokeWeight, rs.strokeCap, rs.strokeJoin, rs.miterLimit, rs.roundJoinThreshold, computeCircleSegmentCount); break;
            case ShapeType::triangleFan: stroke_triangle_fan(writer, pts, rs.strokeWeight, rs.strokeCap, rs.strokeJoin, rs.miterLimit, rs.roundJoinThreshold, computeCircleSegmentCount); break;
            case ShapeType::quads: stroke_quads(writer, pts, rs.strokeWeight, rs.strokeCap, rs.strokeJoin, rs.miterLimit, rs.roundJoinThreshold, computeCircleSegmentCount); break;
            case ShapeType::quadStrip: stroke_quad_strip(writer, pts, rs.strokeWeight, rs.strokeCap, rs.strokeJoin, rs.miterLimit, rs.roundJoinThreshold, computeCircleSegmentCount); break;
            case ShapeType::polygon: stroke_polygon(writer, pts, rs.strokeWeight, rs.strokeCap, rs.strokeJoin, rs.miterLimit, rs.roundJoinThreshold, close, computeCircleSegmentCount); break;
        }

        endDrawOp(writer, m_defaultShader, rs.blendMode, m_whiteTexture, m_uniformCache.getUniforms(m_defaultShader));
    }

} // namespace p5cpp
