#include <p5cpp/p5cpp.hpp>

#include "application/engine.hpp"
#include "application/app_context.hpp"
#include "linepath.hpp"
#include "modules/rendering/rendering_data.hpp"
#include "services/uniform_cache.hpp"
#include "tess.hpp"
#include "utf8_view.hpp"

#include <algorithm>

namespace p5cpp
{
    const StrokeCap StrokeCap::butt = StrokeCap {.start = StrokeCapStyle::butt, .end = StrokeCapStyle::butt};
    const StrokeCap StrokeCap::square = StrokeCap {.start = StrokeCapStyle::square, .end = StrokeCapStyle::square};
    const StrokeCap StrokeCap::round = StrokeCap {.start = StrokeCapStyle::round, .end = StrokeCapStyle::round};

    const TextAlign TextAlign::topLeft = TextAlign {.horizontal = HorizontalTextAlign::left, .vertical = VerticalTextAlign::top};
    const TextAlign TextAlign::topCenter = TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::top};
    const TextAlign TextAlign::topRight = TextAlign {.horizontal = HorizontalTextAlign::right, .vertical = VerticalTextAlign::top};

    const TextAlign TextAlign::centerLeft = TextAlign {.horizontal = HorizontalTextAlign::left, .vertical = VerticalTextAlign::center};
    const TextAlign TextAlign::center = TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::center};
    const TextAlign TextAlign::centerRight = TextAlign {.horizontal = HorizontalTextAlign::right, .vertical = VerticalTextAlign::center};

    const TextAlign TextAlign::bottomLeft = TextAlign {.horizontal = HorizontalTextAlign::left, .vertical = VerticalTextAlign::bottom};
    const TextAlign TextAlign::bottomCenter = TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::bottom};
    const TextAlign TextAlign::bottomRight = TextAlign {.horizontal = HorizontalTextAlign::right, .vertical = VerticalTextAlign::bottom};
} // namespace p5cpp

namespace p5cpp
{
    size_t computeCircleSegmentCount(float angle, float radius)
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
} // namespace p5cpp

namespace p5cpp
{
    extern std::unique_ptr<Engine> engine;

    inline static LinePathBuilder linepath;
    inline static std::array<float2, 4> curveVertexPositions;
    inline static size_t curveVertexCount;
} // namespace p5cpp

namespace p5cpp
{
    inline static RenderStateStack& getRenderStateStack()
    {
        return engine->getContext().require<RenderingData>().renderStateStack;
    }

    inline static Renderer& getRenderer()
    {
        return *engine->getContext().require<RenderingData>().renderer;
    }
} // namespace p5cpp

namespace p5cpp
{
    inline Shader* get_current_shader(RenderingData& data, const RenderState& state)
    {
        if (state.shader != nullptr) {
            return state.shader.get();
        }

        return data.defaultShader.get();
    }

    inline static Font* get_current_font(RenderingData& renderingData, const RenderState& renderState)
    {
        if (renderState.font != nullptr) {
            return renderState.font.get();
        }

        return renderingData.defaultFont.get();
    }
} // namespace p5cpp

namespace p5cpp
{
    // Flush the CPU draw buffer to the GPU if there is less than SAFE_MARGIN
    // room left.  Must be called BEFORE draw_buffer_get_scope() so the new
    // scope always starts at a valid (non-overflowed) base offset.
    void flushIfNeeded(const DrawScope& drawScope)
    {
        constexpr size_t SAFE_MARGIN_V = 4096;
        constexpr size_t SAFE_MARGIN_I = SAFE_MARGIN_V * 3;

        const bool verticesOverflow = drawScope.vertexCursor + SAFE_MARGIN_V >= drawScope.vertices.size();
        const bool indicesOverflow = drawScope.indexCursor + SAFE_MARGIN_I >= drawScope.indices.size();
        if (verticesOverflow or indicesOverflow) {
            RenderingData& renderingData = engine->getContext().require<RenderingData>();
            Renderer& renderer = *renderingData.renderer;
            renderer.flush();
        }
    }
} // namespace p5cpp

namespace p5cpp
{
    void pushCanvas(std::shared_ptr<Framebuffer> canvas)
    {
        RenderingData& renderingData = engine->getContext().require<RenderingData>();
        Renderer& renderer = *renderingData.renderer;
        RenderStateStack& renderStateStack = renderingData.renderStateStack;

        renderer.flush();
        renderer.end();

        Framebuffer* framebuffer = renderingData.framebufferStack.emplace_back(canvas.get());
        renderer.begin(framebuffer);
        render_state_stack_push(renderStateStack, render_state_stack_peek(renderStateStack));
    }

    void popCanvas()
    {
        RenderingData& renderingData = engine->getContext().require<RenderingData>();
        Renderer& renderer = *renderingData.renderer;
        RenderStateStack& renderStateStack = renderingData.renderStateStack;

        renderer.flush();
        renderer.end();

        render_state_stack_pop(renderStateStack);
        renderingData.framebufferStack.pop_back();
        if (not renderingData.framebufferStack.empty()) {
            renderer.begin(renderingData.framebufferStack.back());
        }
    }
} // namespace p5cpp

namespace p5cpp
{
    void background(int grey, int alpha) { background(rgba(grey, alpha)); }
    void background(int red, int green, int blue, int alpha) { background(rgba(red, green, blue, alpha)); }
    void background(color_t color)
    {
        RenderingData& renderingData = engine->getContext().require<RenderingData>();
        Renderer& renderer = *renderingData.renderer;
        UniformCache& uniformCache = *renderingData.uniformCache;
        Texture* whiteTexture = renderingData.whiteTexture.get();

        const RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        const uint2 size = renderingData.framebufferStack.back()->getSize();
        const std::array<float2, 4> positions = {
            float2 {0.0f, 0.0f},
            float2 {static_cast<float>(size.x), 0.0f},
            float2 {static_cast<float>(size.x), static_cast<float>(size.y)},
            float2 {0.0f, static_cast<float>(size.y)},
        };
        const std::array<float2, 4> texcoords = {
            float2 {0.0f, 0.0f},
            float2 {0.0f, 0.0f},
            float2 {0.0f, 0.0f},
            float2 {0.0f, 0.0f},
        };
        const std::array<color_t, 4> colors = {
            color,
            color,
            color,
            color,
        };

        {
            // flushIfNeeded();
            DrawScope scope = getRenderer().getDrawScope();

            tesselate_quads(
                scope,
                PathPoints {
                    .size = 4,
                    .positions = positions,
                    .texcoords = texcoords,
                    .colors = colors,
                }
            );

            renderer.submit(
                scope,
                uniformCache,
                get_current_shader(renderingData, renderState),
                renderState.blendMode,
                whiteTexture
            );
        }
    }
} // namespace p5cpp

namespace p5cpp
{
    void endShapeImpl(ShapeType fillType, ShapeType strokeType, std::optional<ColorStyle> fillStyle, std::optional<ColorStyle> strokeStyle, bool close)
    {
        RenderingData& renderingData = engine->getContext().require<RenderingData>();
        Renderer& renderer = *renderingData.renderer;
        RenderState& renderState = render_state_stack_peek(renderingData.renderStateStack);
        UniformCache& uniformCache = *renderingData.uniformCache;

        DrawScope scope = renderer.getDrawScope();
        flushIfNeeded(scope);

        if (fillStyle.has_value()) {
            const PathPoints points = linepath.buildDrawPoints(fillStyle.value());

            switch (fillType) {
                case ShapeType::lines: break;
                case ShapeType::lineStrip: break;
                case ShapeType::lineLoop: break;
                case ShapeType::triangles: tesselate_triangles(scope, points); break;
                case ShapeType::triangleStrip: tesselate_triangle_strip(scope, points); break;
                case ShapeType::triangleFan: tesselate_triangle_fan(scope, points); break;
                case ShapeType::quads: tesselate_quads(scope, points); break;
                case ShapeType::quadStrip: tesselate_quad_strip(scope, points); break;
                case ShapeType::polygon: tesselate_polygon(scope, points); break;
            }
        }

        if (strokeStyle.has_value()) {
            const PathPoints points = linepath.buildDrawPoints(strokeStyle.value());

            switch (strokeType) {
                case ShapeType::lines: stroke_lines(scope, points, renderState.strokeWeight, renderState.strokeCap, renderState.miterLimit, renderState.roundJoinThreshold); break;
                case ShapeType::lineStrip: stroke_line_strip(scope, points, renderState.strokeWeight, renderState.strokeCap, renderState.strokeJoin, renderState.miterLimit, renderState.roundJoinThreshold); break;
                case ShapeType::lineLoop: stroke_line_loop(scope, points, renderState.strokeWeight, renderState.strokeJoin, renderState.miterLimit, renderState.roundJoinThreshold); break;
                case ShapeType::triangles: stroke_triangles(scope, points, renderState.strokeWeight, renderState.strokeCap, renderState.strokeJoin, renderState.miterLimit, renderState.roundJoinThreshold); break;
                case ShapeType::triangleStrip: stroke_triangle_strip(scope, points, renderState.strokeWeight, renderState.strokeCap, renderState.strokeJoin, renderState.miterLimit, renderState.roundJoinThreshold); break;
                case ShapeType::triangleFan: stroke_triangle_fan(scope, points, renderState.strokeWeight, renderState.strokeCap, renderState.strokeJoin, renderState.miterLimit, renderState.roundJoinThreshold); break;
                case ShapeType::quads: stroke_quads(scope, points, renderState.strokeWeight, renderState.strokeCap, renderState.strokeJoin, renderState.miterLimit, renderState.roundJoinThreshold); break;
                case ShapeType::quadStrip: stroke_quad_strip(scope, points, renderState.strokeWeight, renderState.strokeCap, renderState.strokeJoin, renderState.miterLimit, renderState.roundJoinThreshold); break;
                case ShapeType::polygon: stroke_polygon(scope, points, renderState.strokeWeight, renderState.strokeCap, renderState.strokeJoin, renderState.miterLimit, renderState.roundJoinThreshold, close); break;
            }
        }

        renderer.submit(scope, uniformCache, get_current_shader(renderingData, renderState), renderState.blendMode, renderingData.whiteTexture.get());
        linepath.clear();
        curveVertexCount = 0;
    }

    void beginShape()
    {
    }

    void endShape(ShapeType type, bool close)
    {
        RenderingData& renderingData = engine->getContext().require<RenderingData>();
        RenderState& renderState = render_state_stack_peek(renderingData.renderStateStack);
        const std::optional<ColorStyle> fillStyle = renderState.isFillDisabled ? std::nullopt : std::make_optional(ColorStyle::fill);
        const std::optional<ColorStyle> strokeStyle = renderState.isStrokeDisabled ? std::nullopt : std::make_optional(ColorStyle::stroke);
        endShapeImpl(type, type, fillStyle, strokeStyle, close);
    }

    void vertex(float x, float y)
    {
        vertex(x, y, 0.0f, 0.0f);
    }

    void vertex(float x, float y, float u, float v)
    {
        RenderingData& renderingData = engine->getContext().require<RenderingData>();
        RenderStateStack& renderStateStack = renderingData.renderStateStack;
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        const matrix4x4& matrix = matrix_stack_peek(renderState.metrics);
        const float2 transformed = matrix.transformPoint(x, y);

        linepath.vertex(transformed.x, transformed.y, u, v, renderState.fillColor, renderState.strokeColor);
    }

    void curveVertex(float x, float y)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        curveVertexPositions[curveVertexCount++] = {x, y};

        if (curveVertexCount >= 4) {
            const float alpha = (1.0f - renderState.curveTightness) * 0.5f;

            for (size_t i = 0; i < curveVertexPositions.size() - 3; ++i) {
                auto [x1, y1] = curveVertexPositions[i];
                auto [x2, y2] = curveVertexPositions[i + 1];
                auto [x3, y3] = curveVertexPositions[i + 2];
                auto [x4, y4] = curveVertexPositions[i + 3];

                for (size_t j = 0; j <= renderState.curveDetail; ++j) {
                    float t = static_cast<float>(j) * renderState.invCurveDetail;
                    float t2 = t * t;
                    float t3 = t2 * t;

                    float bx = alpha * ((-x1 + 3 * x2 - 3 * x3 + x4) * t3 + (2 * x1 - 5 * x2 + 4 * x3 - x4) * t2 + (-x1 + x3) * t) + x2;
                    float by = alpha * ((-y1 + 3 * y2 - 3 * y3 + y4) * t3 + (2 * y1 - 5 * y2 + 4 * y3 - y4) * t2 + (-y1 + y3) * t) + y2;

                    vertex(bx, by);
                }
            }

            curveVertexCount = 0;
        }
    }
} // namespace p5cpp

namespace p5cpp
{
    void rect(float left, float top, float width, float height)
    {
        beginShape();
        vertex(left, top);
        vertex(left + width, top);
        vertex(left + width, top + height);
        vertex(left, top + height);
        endShape(ShapeType::quads, true);
    }

    void rect(float left, float top, float width, float height, float cx, float cy)
    {
        rect(left, top, width, height, cx, cy, cx, cy, cx, cy, cx, cy);
    }

    void rect(float left, float top, float width, float height, float topLeftX, float topLeftY, float topRightX, float topRightY, float bottomRightX, float bottomRightY, float bottomLeftX, float bottomLeftY)
    {
        float maxRx = width * 0.5f;
        float maxRy = height * 0.5f;
        topLeftX = std::min(topLeftX, maxRx);
        topLeftY = std::min(topLeftY, maxRy);
        topRightX = std::min(topRightX, maxRx);
        topRightY = std::min(topRightY, maxRy);
        bottomRightX = std::min(bottomRightX, maxRx);
        bottomRightY = std::min(bottomRightY, maxRy);
        bottomLeftX = std::min(bottomLeftX, maxRx);
        bottomLeftY = std::min(bottomLeftY, maxRy);

        struct Corner
        {
            float cx, cy, rx, ry, startAngle;
        };

        const Corner corners[4] = {
            {left + width - bottomRightX, top + height - bottomRightY, bottomRightX, bottomRightY, 0.0f}, // unten rechts
            {left + bottomLeftX, top + height - bottomLeftY, bottomLeftX, bottomLeftY, HALF_PI},          // unten links
            {left + topLeftX, top + topLeftY, topLeftX, topLeftY, HALF_PI * 2},                           // oben links
            {left + width - topRightX, top + topRightY, topRightX, topRightY, HALF_PI * 3},               // oben rechts
        };

        // Emits all arc vertices for all 4 corners.
        // Each arc uses i < segs (open end) so consecutive corners share no duplicate junction vertex.
        // Zero-radius corners clamp to 1 segment, which emits exactly the corner point.
        auto emitRim = [&] {
            for (const auto& c : corners) {
                const size_t segs = std::max(size_t(1), computeCircleSegmentCount(HALF_PI, std::max(c.rx, c.ry)));
                for (size_t i = 0; i < segs; ++i) {
                    const float angle = c.startAngle + HALF_PI * (float(i) / float(segs));
                    vertex(c.cx + std::cos(angle) * c.rx, c.cy + std::sin(angle) * c.ry);
                }
            }
        };

        RenderState& renderState = render_state_stack_peek(getRenderStateStack());

        // Fill: triangleFan from the centre – rounded rect is always convex, so no libtess2 needed.
        if (!renderState.isFillDisabled) {
            beginShape();
            vertex(left + width * 0.5f, top + height * 0.5f); // fan centre
            emitRim();
            vertex(corners[0].cx + corners[0].rx, corners[0].cy); // re-emit first rim vertex to close the fan
            endShapeImpl(ShapeType::triangleFan, ShapeType::triangleFan, ColorStyle::fill, std::nullopt, false);
        }

        // Stroke: lineLoop around the rim only (no internal fan edges).
        if (!renderState.isStrokeDisabled) {
            beginShape();
            emitRim();
            endShapeImpl(ShapeType::lineLoop, ShapeType::lineLoop, std::nullopt, ColorStyle::stroke, false);
        }
    }

    void square(float left, float top, float size)
    {
        rect(left, top, size, size);
    }

    void ellipse(float centerX, float centerY, float width, float height)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        const size_t segmentCount = computeCircleSegmentCount(TWO_PI, std::max(width, height) * 0.5f);

        // Fill: centre vertex + closed rim → triangleFan (no libtess2, O(n) vertices)
        if (!renderState.isFillDisabled) {
            beginShape();
            vertex(centerX, centerY);
            for (size_t i = 0; i <= segmentCount; ++i) {
                const float angle = TWO_PI / static_cast<float>(segmentCount) * static_cast<float>(i);
                vertex(centerX + std::cos(angle) * width * 0.5f, centerY + std::sin(angle) * height * 0.5f);
            }
            endShapeImpl(ShapeType::triangleFan, ShapeType::triangleFan, ColorStyle::fill, std::nullopt, false);
        }

        // Stroke: rim only → lineLoop, so only the outer edge is stroked, not the internal fan edges
        if (!renderState.isStrokeDisabled) {
            beginShape();
            for (size_t i = 0; i < segmentCount; ++i) {
                const float angle = TWO_PI / static_cast<float>(segmentCount) * static_cast<float>(i);
                vertex(centerX + std::cos(angle) * width * 0.5f, centerY + std::sin(angle) * height * 0.5f);
            }
            endShapeImpl(ShapeType::lineLoop, ShapeType::lineLoop, std::nullopt, ColorStyle::stroke, false);
        }
    }

    void circle(float centerX, float centerY, float size)
    {
        ellipse(centerX, centerY, size, size);
    }

    void point(float x, float y)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        const size_t segmentCount = computeCircleSegmentCount(TWO_PI, renderState.strokeWeight * 0.5f);

        // Fill a disc with the stroke color as a triangleFan — no libtess2, no outline edges
        beginShape();
        vertex(x, y);
        for (size_t i = 0; i <= segmentCount; ++i) {
            const float angle = TWO_PI / static_cast<float>(segmentCount) * static_cast<float>(i);
            vertex(x + std::cos(angle) * renderState.strokeWeight * 0.5f, y + std::sin(angle) * renderState.strokeWeight * 0.5f);
        }
        endShapeImpl(ShapeType::triangleFan, ShapeType::triangleFan, ColorStyle::stroke, std::nullopt, false);
    }

    void triangle(float x1, float y1, float x2, float y2, float x3, float y3)
    {
        beginShape();
        vertex(x1, y1);
        vertex(x2, y2);
        vertex(x3, y3);
        endShape(ShapeType::triangles, true);
    }

    void line(float x1, float y1, float x2, float y2)
    {
        beginShape();
        vertex(x1, y1);
        vertex(x2, y2);
        endShapeImpl(ShapeType::lines, ShapeType::lines, std::nullopt, ColorStyle::stroke, false);
    }

    void arc(float centerX, float centerY, float width, float height, float startAngle, float sweepAngle, ArcMode arcMode)
    {
        const float radius = std::max(width, height) * 0.5f;
        const size_t segmentCount = computeCircleSegmentCount(sweepAngle, radius);

        beginShape();

        if (arcMode == ArcMode::pie) {
            vertex(centerX, centerY);
        }

        for (size_t i = 0; i <= segmentCount; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(segmentCount);
            float angle = startAngle + t * sweepAngle;
            float x = centerX + std::cos(angle) * width * 0.5f;
            float y = centerY + std::sin(angle) * height * 0.5f;
            vertex(x, y);
        }

        endShape(ShapeType::polygon, arcMode != ArcMode::open);
    }

    void bezier(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());

        beginShape();
        for (size_t i = 0; i <= renderState.bezierDetail; ++i) {
            float t = static_cast<float>(i) * renderState.invBezierDetail;

            float mt = 1.0f - t;
            float mt2 = mt * mt;
            float mt3 = mt2 * mt;
            float t2 = t * t;
            float t3 = t2 * t;

            float bx = mt3 * x1 + 3 * mt2 * t * x2 + 3 * mt * t2 * x3 + t3 * x4;
            float by = mt3 * y1 + 3 * mt2 * t * y2 + 3 * mt * t2 * y3 + t3 * y4;

            vertex(bx, by);
        }
        endShapeImpl(ShapeType::lineStrip, ShapeType::lineStrip, std::nullopt, ColorStyle::stroke, false);
    }

    void curve(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());

        float alpha = (1.0f - renderState.curveTightness) * 0.5f;

        beginShape();
        for (size_t i = 0; i <= renderState.curveDetail; ++i) {
            float t = static_cast<float>(i) * renderState.invCurveDetail;
            float t2 = t * t;
            float t3 = t2 * t;

            float bx = alpha * ((-x1 + 3 * x2 - 3 * x3 + x4) * t3 + (2 * x1 - 5 * x2 + 4 * x3 - x4) * t2 + (-x1 + x3) * t) + x2;
            float by = alpha * ((-y1 + 3 * y2 - 3 * y3 + y4) * t3 + (2 * y1 - 5 * y2 + 4 * y3 - y4) * t2 + (-y1 + y3) * t) + y2;

            vertex(bx, by);
        }
        endShapeImpl(ShapeType::lineStrip, ShapeType::lineStrip, std::nullopt, ColorStyle::stroke, false);
    }
} // namespace p5cpp

namespace p5cpp
{
    void image(Texture* texture, float left, float top, float width, float height)
    {
        RenderingData& renderingData = engine->getContext().require<RenderingData>();
        Renderer& renderer = *renderingData.renderer;
        RenderStateStack& renderStateStack = renderingData.renderStateStack;
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        UniformCache& uniformCache = *renderingData.uniformCache;

        const matrix4x4& matrix = matrix_stack_peek(renderState.metrics);

        const std::array<float2, 4> positions = {
            matrix.transformPoint(left, top),
            matrix.transformPoint(left + width, top),
            matrix.transformPoint(left + width, top + height),
            matrix.transformPoint(left, top + height),
        };
        const std::array<float2, 4> texcoords = {
            float2 {0.0f, 1.0f},
            float2 {1.0f, 1.0f},
            float2 {1.0f, 0.0f},
            float2 {0.0f, 0.0f}
        };
        const std::array<color_t, 4> colors = {
            renderState.tintColor,
            renderState.tintColor,
            renderState.tintColor,
            renderState.tintColor
        };

        {
            DrawScope scope = renderer.getDrawScope();
            flushIfNeeded(scope);

            tesselate_quads(
                scope,
                PathPoints {
                    .size = 4,
                    .positions = positions,
                    .texcoords = texcoords,
                    .colors = colors,
                }
            );

            renderer.submit(scope, uniformCache, get_current_shader(renderingData, renderState), renderState.blendMode, texture);
        }
    }
} // namespace p5cpp

namespace p5cpp
{

    TextLayout measureText(std::string_view text)
    {
        RenderingData& renderingData = engine->getContext().require<RenderingData>();
        RenderStateStack& renderStateStack = renderingData.renderStateStack;
        RenderState& renderState = render_state_stack_peek(renderStateStack);

        return measureText(
            text,
            get_current_font(renderingData, renderState),
            renderState.textSize,
            renderState.textLetterSpacing,
            renderState.textLineSpacing,
            renderState.textAlign,
            renderState.textWrap,
            std::nullopt
        );
    }

    static float measureLineWidth(std::u32string_view source, size_t start, size_t end, Font* font, float textSize, float letterSpacing)
    {
        float width = 0.0f;

        for (size_t i = start; i < end; ++i) {
            const Glyph* glyph = font->getGlyph(source[i], textSize);
            if (glyph == nullptr) {
                continue;
            }

            const bool hasPrevious = i > start;
            if (hasPrevious) {
                const char32_t currentCodepoint = source[i];
                const char32_t previousCodepoint = source[i - 1];
                const float kerning = font->getKerning(previousCodepoint, currentCodepoint, textSize);

                width += kerning;
            }

            const float advance = glyph->advanceX + letterSpacing;
            width += advance;
        }

        const bool isNotEmpty = end > start;
        if (isNotEmpty) {
            width -= letterSpacing;
        }

        return width;
    }

    struct WordRange
    {
        size_t start;
        size_t end;
    };

    std::vector<WordRange> splitLineIntoWords(const std::u32string_view source, size_t lineStart, size_t lineEnd)
    {
        std::vector<WordRange> words;

        size_t cursor = lineStart;
        while (cursor < lineEnd) {
            // Skip white spaces
            while (cursor < lineEnd && source.at(cursor) == U' ') {
                ++cursor;
            }

            size_t wordEnd = cursor;
            while (wordEnd < lineEnd && source.at(wordEnd) != U' ') {
                ++wordEnd;
            }

            words.push_back(WordRange {
                .start = cursor,
                .end = wordEnd,
            });

            cursor = wordEnd;
        }

        return words;
    }

    struct LineInfo
    {
        size_t startIndex;
        size_t endIndex;
        float width;
    };

    std::vector<LineInfo> splitTextIntoLines(std::u32string_view source, Font* font, TextWrap textWrap, std::optional<float> maxWidth, float textSize, float letterSpacing)
    {
        std::vector<LineInfo> lines;
        size_t segmentStart = 0;

        for (size_t i = 0; i <= source.length(); ++i) {
            const bool isEnd = i == source.length();
            const bool isNewline = not isEnd and (source[i] == U'\n');

            if (not isEnd and not isNewline) {
                continue;
            }

            // --- Segment [segmentStart, i) verarbeiten ---

            if (not maxWidth.has_value() or textWrap == TextWrap::none) {
                const float width = measureLineWidth(source, segmentStart, i, font, textSize, letterSpacing);
                lines.push_back(LineInfo {
                    .startIndex = segmentStart,
                    .endIndex = i,
                    .width = width,
                });
            } else if (textWrap == TextWrap::character) {
                size_t lineStart = segmentStart;
                float lineWidth = 0.0f;

                for (size_t j = segmentStart; j < i; ++j) {
                    const char32_t currentCodepoint = source[j];
                    const Glyph* glyph = font->getGlyph(currentCodepoint, textSize);
                    if (glyph == nullptr) {
                        continue;
                    }

                    const bool isFirstInLine = (j == lineStart);
                    float kerningAdjustment = 0.0f;
                    if (not isFirstInLine) {
                        const char32_t previousCodepoint = source[j - 1];
                        kerningAdjustment = font->getKerning(previousCodepoint, currentCodepoint, textSize);
                    }
                    const float advance = glyph->advanceX + letterSpacing + kerningAdjustment;

                    if (not isFirstInLine and (lineWidth + advance > maxWidth.value())) {
                        lines.push_back(LineInfo {
                            .startIndex = lineStart,
                            .endIndex = j,
                            .width = lineWidth,
                        });
                        lineStart = j;
                        // Erstes Zeichen der neuen Zeile: kein Kerning zum Vorgänger der alten Zeile.
                        lineWidth = glyph->advanceX + letterSpacing;
                    } else {
                        lineWidth += advance;
                    }
                }

                lines.push_back(LineInfo {
                    .startIndex = lineStart,
                    .endIndex = i,
                    .width = lineWidth,
                });
            } else if (textWrap == TextWrap::word) {
                const std::vector<WordRange> words = splitLineIntoWords(source, segmentStart, i);
                size_t lineStart = segmentStart;
                size_t lastWordEnd = segmentStart;

                for (size_t w = 0; w < words.size(); ++w) {
                    const WordRange& word = words[w];
                    const bool isFirstWord = (word.start == lineStart);

                    // Tatsächliche Breite der Zeile, WENN dieses Wort noch mit angehängt wird.
                    const float candidateWidth = measureLineWidth(source, lineStart, word.end, font, textSize, letterSpacing);

                    if (not isFirstWord and candidateWidth > maxWidth.value()) {
                        const float lineWidth = measureLineWidth(source, lineStart, lastWordEnd, font, textSize, letterSpacing);
                        lines.push_back(LineInfo {
                            .startIndex = lineStart,
                            .endIndex = word.start,
                            .width = lineWidth,
                        });
                        lineStart = word.start;
                    }

                    lastWordEnd = word.end;
                }

                const float lineWidth = measureLineWidth(source, lineStart, i, font, textSize, letterSpacing);
                lines.push_back(LineInfo {
                    .startIndex = lineStart,
                    .endIndex = i,
                    .width = lineWidth,
                });
            }

            segmentStart = i + 1;
        }

        return lines;
    }

    TextLayout measureText(std::string_view text, Font* font, float textSize, float letterSpacing, float lineSpacing, TextAlign textAlign, TextWrap textWrap, std::optional<float> maxWidth)
    {
        const std::u32string u32Text = utf8ToUtf32(text);
        const std::vector<LineInfo> lines = splitTextIntoLines(u32Text, font, textWrap, maxWidth, textSize, letterSpacing);

        const FontMetrics* fontMetrics = font->getMetrics(textSize);
        const float lineStep = fontMetrics->lineHeight * lineSpacing;
        const float totalHeight = lineStep * static_cast<float>(lines.size() - 1) + fontMetrics->lineHeight;

        const float originY = std::invoke([textAlign, totalHeight, ascender = fontMetrics->ascender]() {
            switch (textAlign.vertical) {
                case VerticalTextAlign::top: return ascender;
                case VerticalTextAlign::center: return ascender - totalHeight * 0.5f;
                case VerticalTextAlign::bottom: return ascender - totalHeight;
                case VerticalTextAlign::baseline: return 0.0f;
            }
        });

        std::vector<GlyphQuad> quads;
        std::vector<LineLayout> lineLayouts;
        lineLayouts.reserve(lines.size());

        for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
            const LineInfo& line = lines.at(lineIndex);

            const float originX = std::invoke([textAlign, line]() {
                switch (textAlign.horizontal) {
                    case HorizontalTextAlign::left: return 0.0f;
                    case HorizontalTextAlign::center: return -line.width * 0.5f;
                    case HorizontalTextAlign::right: return -line.width;
                }
            });

            float cursorX = originX;
            float cursorY = originY + lineStep * static_cast<float>(lineIndex);

            lineLayouts.push_back(LineLayout {
                .codepointsStart = line.startIndex,
                .codepointsEnd = line.endIndex,
                .width = line.width,
                .y = cursorY,
            });

            for (size_t i = line.startIndex; i < line.endIndex; ++i) {
                if (i > line.startIndex) {
                    cursorX += font->getKerning(u32Text.at(i - 1), u32Text.at(i), textSize);
                }

                const Glyph* glyph = font->getGlyph(u32Text.at(i), textSize);
                if (glyph == nullptr) continue;

                if (glyph->region.size.x == 0 or glyph->region.size.y == 0) {
                    cursorX += glyph->advanceX + letterSpacing;
                    continue;
                }

                quads.push_back(GlyphQuad {
                    .vertexRect = rect2f {
                        .left = cursorX + glyph->bearing.x,
                        .top = cursorY - glyph->bearing.y,
                        .width = static_cast<float>(glyph->region.size.x),
                        .height = static_cast<float>(glyph->region.size.y),
                    },
                    .uvRect = glyph->region.uvRect,
                    .texture = font->getGlyphAtlasTexture(glyph->glyphAtlasIndex),
                    .codepointIndex = i,
                });

                cursorX += glyph->advanceX + letterSpacing;
            }
        }

        const float totalWidth = std::invoke([&lines]() {
            float maxWidth = 0.0f;
            for (const LineInfo& line : lines) {
                maxWidth = std::max(maxWidth, line.width);
            }
            return maxWidth;
        });

        return TextLayout {
            .totalWidth = totalWidth,
            .totalHeight = totalHeight,
            .ascender = fontMetrics->ascender,
            .descender = fontMetrics->descender,
            .glyphs = std::move(quads),
            .lines = std::move(lineLayouts),
        };
    }

    void text(std::string_view text, float x, float y, std::optional<float> maxWidth)
    {
        RenderingData& renderingData = engine->getContext().require<RenderingData>();
        Renderer& renderer = *renderingData.renderer;
        RenderStateStack& renderStateStack = renderingData.renderStateStack;
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        UniformCache& uniformCache = *renderingData.uniformCache;
        Font* font = get_current_font(renderingData, renderState);
        Shader* textShader = (renderState.shader != nullptr) ? renderState.shader.get() : renderingData.textShader.get();
        TextLayout layout = measureText(text, font, renderState.textSize, renderState.textLetterSpacing, renderState.textLineSpacing, renderState.textAlign, renderState.textWrap, maxWidth);
        matrix4x4& matrix = matrix_stack_peek(renderState.metrics);

        struct GlyphAtlasVertexBucket
        {
            std::vector<float2> positions;
            std::vector<float2> texcoords;
            std::vector<color_t> colors;
        };

        std::unordered_map<Texture*, GlyphAtlasVertexBucket> buckets;

        for (size_t i = 0; i < layout.glyphs.size(); ++i) {
            const GlyphQuad& quad = layout.glyphs.at(i);
            const auto itr = buckets.try_emplace(quad.texture, GlyphAtlasVertexBucket {});
            GlyphAtlasVertexBucket& bucket = itr.first->second;

            bucket.positions.push_back(matrix.transformPoint(x + quad.vertexRect.left, y + quad.vertexRect.top));
            bucket.positions.push_back(matrix.transformPoint(x + quad.vertexRect.left + quad.vertexRect.width, y + quad.vertexRect.top));
            bucket.positions.push_back(matrix.transformPoint(x + quad.vertexRect.left + quad.vertexRect.width, y + quad.vertexRect.top + quad.vertexRect.height));
            bucket.positions.push_back(matrix.transformPoint(x + quad.vertexRect.left, y + quad.vertexRect.top + quad.vertexRect.height));

            bucket.texcoords.push_back(float2 {quad.uvRect.left, quad.uvRect.top});
            bucket.texcoords.push_back(float2 {quad.uvRect.left + quad.uvRect.width, quad.uvRect.top});
            bucket.texcoords.push_back(float2 {quad.uvRect.left + quad.uvRect.width, quad.uvRect.top + quad.uvRect.height});
            bucket.texcoords.push_back(float2 {quad.uvRect.left, quad.uvRect.top + quad.uvRect.height});

            bucket.colors.push_back(renderState.fillColor);
            bucket.colors.push_back(renderState.fillColor);
            bucket.colors.push_back(renderState.fillColor);
            bucket.colors.push_back(renderState.fillColor);
        }

        for (const auto& [texture, bucket] : buckets) {
            DrawScope scope = getRenderer().getDrawScope();
            flushIfNeeded(scope);

            tesselate_quads(
                scope,
                PathPoints {
                    .size = static_cast<uint32_t>(bucket.positions.size()),
                    .positions = bucket.positions,
                    .texcoords = bucket.texcoords,
                    .colors = bucket.colors,
                }
            );

            renderer.submit(scope, uniformCache, textShader, renderState.blendMode, texture);
        }
    }
} // namespace p5cpp
