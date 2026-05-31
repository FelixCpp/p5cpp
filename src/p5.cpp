#include "p5.hpp"
#include "window.hpp"
#include "shader.hpp"
#include "canvas.hpp"
#include "rendering.hpp"
#include "linepath.hpp"
#include "renderstate.hpp"
#include "tess.hpp"
#include "dejavusans.hpp"
#include "uniform_cache.hpp"
#include "timing.hpp"
#include <cassert>

#include <iostream>
#include <array>
#include <numbers>
#include <algorithm>
#include <chrono>
#include <thread>

namespace p5
{
    const StrokeCap StrokeCap::butt = StrokeCap {.start = StrokeCapStyle::butt, .end = StrokeCapStyle::butt};
    const StrokeCap StrokeCap::square = StrokeCap {.start = StrokeCapStyle::square, .end = StrokeCapStyle::square};
    const StrokeCap StrokeCap::round = StrokeCap {.start = StrokeCapStyle::round, .end = StrokeCapStyle::round};
} // namespace p5

namespace p5
{
    // Decodes the next UTF-8 codepoint from sv starting at byte index pos.
    // Advances pos past all consumed bytes.
    static char32_t utf8NextCodepoint(std::string_view sv, size_t& pos)
    {
        const auto b0 = static_cast<unsigned char>(sv[pos]);
        if (b0 < 0x80) {
            ++pos;
            return static_cast<char32_t>(b0);
        }
        if ((b0 & 0xE0) == 0xC0 && pos + 1 < sv.size()) {
            const auto b1 = static_cast<unsigned char>(sv[pos + 1]);
            pos += 2;
            return static_cast<char32_t>(((b0 & 0x1F) << 6) | (b1 & 0x3F));
        }
        if ((b0 & 0xF0) == 0xE0 && pos + 2 < sv.size()) {
            const auto b1 = static_cast<unsigned char>(sv[pos + 1]);
            const auto b2 = static_cast<unsigned char>(sv[pos + 2]);
            pos += 3;
            return static_cast<char32_t>(((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F));
        }
        if ((b0 & 0xF8) == 0xF0 && pos + 3 < sv.size()) {
            const auto b1 = static_cast<unsigned char>(sv[pos + 1]);
            const auto b2 = static_cast<unsigned char>(sv[pos + 2]);
            const auto b3 = static_cast<unsigned char>(sv[pos + 3]);
            pos += 4;
            return static_cast<char32_t>(((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F));
        }
        // Invalid or truncated sequence: skip byte
        ++pos;
        return U'\uFFFD';
    }

    color_t color(int grey, int alpha) { return color(grey, grey, grey, alpha); }
    color_t color(int red, int green, int blue, int alpha) { return (red << 24) | (green << 16) | (blue << 8) | alpha; } // TODO(Felix): Clamp values to 0 .. 255
    color_t lerp(color_t a, color_t b, float t)
    {
        return color(
            static_cast<int>(red(a) + t * (red(b) - red(a))),
            static_cast<int>(green(a) + t * (green(b) - green(a))),
            static_cast<int>(blue(a) + t * (blue(b) - blue(a))),
            static_cast<int>(alpha(a) + t * (alpha(b) - alpha(a)))
        );
    }

    int red(color_t color) { return (color & 0xFF000000) >> 24; }
    int green(color_t color) { return (color & 0x00FF0000) >> 16; }
    int blue(color_t color) { return (color & 0x0000FF00) >> 8; }
    int alpha(color_t color) { return (color & 0x000000FF) >> 0; }
    int brightness(color_t color)
    {
        const int r = red(color);
        const int g = green(color);
        const int b = blue(color);

        return static_cast<int>(0.299f * r + 0.587f * g + 0.114f * b);
    }

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

} // namespace p5

namespace p5
{
    void info(std::string_view message) { std::cout << "[INFO]: " << message << std::endl; }
    void debug(std::string_view message) { std::cout << "[DEBUG]: " << message << std::endl; }
    void warning(std::string_view message) { std::cout << "[WARNING]: " << message << std::endl; }
    void error(std::string_view message) { std::cerr << "[ERROR]: " << message << std::endl; }
} // namespace p5

namespace p5
{
    inline static size_t curveVertexCount;
    inline static std::array<float2, 4> curveVertexPositions;

    inline static std::unique_ptr<LinePathBuilder> linepath;
    inline static std::shared_ptr<Shader> defaultShader;
    inline static std::shared_ptr<Shader> textShader;
    inline static std::shared_ptr<Font> defaultFont;
    inline static std::shared_ptr<Framebuffer> defaultFramebuffer;
    inline static std::unique_ptr<Texture> whiteTexture;
    inline static CanvasStack canvasStack;
    inline static Renderer renderer;
    inline static DrawBuffer drawBuffer;
    inline static UniformCache uniformCache;

    inline static AppWindow* appWindow = nullptr;

    inline RenderStateStack& get_render_state_stack() { return canvas_stack_peek(canvasStack).renderStates; }
    inline RenderState& peekState() { return render_state_stack_peek(get_render_state_stack()); }
    inline std::shared_ptr<Shader> getCurrentShader(const RenderState& state)
    {
        auto result = (state.shader != nullptr) ? state.shader : defaultShader;
        assert(result != nullptr && "Current shader cannot be null");
        return result;
    }

    inline std::shared_ptr<Font> getCurrentFont(const RenderState& state)
    {
        auto result = (state.font != nullptr) ? state.font : defaultFont;
        assert(result != nullptr && "Current font cannot be null");
        return result;
    }

    UniformVariable uniform(float x) { return UniformVariable {.type = UniformVariable::Type::float1, .floatValue = x}; }
    UniformVariable uniform(float x, float y) { return UniformVariable {.type = UniformVariable::Type::float2, .float2Value = float2 {x, y}}; }
    UniformVariable uniform(float x, float y, float z, float w) { return UniformVariable {.type = UniformVariable::Type::float4, .float4Value = float4 {x, y, z, w}}; }
    UniformVariable uniform(const matrix4x4& value) { return UniformVariable {.type = UniformVariable::Type::matrix4x4, .matrix4x4Value = value}; }

    // Flush the CPU draw buffer to the GPU if there is less than SAFE_MARGIN
    // room left.  Must be called BEFORE draw_buffer_get_scope() so the new
    // scope always starts at a valid (non-overflowed) base offset.
    void flushIfNeeded()
    {
        constexpr size_t SAFE_MARGIN_V = 4096;
        constexpr size_t SAFE_MARGIN_I = SAFE_MARGIN_V * 3;
        if (drawBuffer.vertexCursor + SAFE_MARGIN_V >= drawBuffer.vertexCount ||
            drawBuffer.indexCursor + SAFE_MARGIN_I >= drawBuffer.indexCount) {
            Canvas& canvas = canvas_stack_peek(canvasStack);
            renderer_flush(renderer, uniformCache, canvas, drawBuffer);
        }
    }

    void pushCanvas(std::shared_ptr<Framebuffer> framebuffer)
    {
        // First we need to flush the renderer to make sure that all draw calls for the current canvas are submitted before we switch to the new canvas.
        if (not canvas_stack_is_empty(canvasStack)) {
            Canvas& canvas = canvas_stack_peek(canvasStack);
            renderer_flush(renderer, uniformCache, canvas, drawBuffer);
        }

        canvas_stack_push(
            canvasStack,
            Canvas {
                .framebuffer = std::move(framebuffer),
                .renderStates = render_state_stack_create(),
                .drawCommands = {},
            }
        );
    }

    void popCanvas()
    {
        // FLush the current canvas before we pop it, to make sure that all draw calls for the current canvas are submitted before we switch back to the previous canvas.
        if (not canvas_stack_is_empty(canvasStack)) {
            Canvas& canvas = canvas_stack_peek(canvasStack);
            renderer_flush(renderer, uniformCache, canvas, drawBuffer);
        }

        canvas_stack_pop(canvasStack);
    }

    void pushState() { render_state_stack_push(get_render_state_stack(), peekState()); }
    void popState() { render_state_stack_pop(get_render_state_stack()); }

    void pushMatrix() { matrix_stack_push(peekState().metrics, matrix_stack_peek(peekState().metrics)); }
    void popMatrix() { matrix_stack_pop(peekState().metrics); }
    void resetMatrix() { matrix_stack_reset(peekState().metrics); }
    void applyMatrix(const matrix4x4& matrix) { matrix_stack_apply(peekState().metrics, matrix); }
    void setMatrix(const matrix4x4& matrix) { matrix_stack_set(peekState().metrics, matrix); }
    matrix4x4& peekMatrix() { return matrix_stack_peek(peekState().metrics); }
    void translate(float x, float y) { applyMatrix(translation(x, y)); }
    void scale(float x, float y) { applyMatrix(scaling(x, y)); }
    void rotate(float angle) { applyMatrix(rotation(angle)); }

    void background(int grey, int alpha) { background(color(grey, alpha)); }
    void background(int red, int green, int blue, int alpha) { background(color(red, green, blue, alpha)); }
    void background(color_t color)
    {
        RenderState& state = peekState();
        const uint2 size = canvas_stack_peek(canvasStack).framebuffer->getSize();
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
            flushIfNeeded();
            DrawScope scope = draw_buffer_get_scope(drawBuffer);

            tesselate_quads(
                scope,
                PathPoints {
                    .size = 4,
                    .positions = positions,
                    .texcoords = texcoords,
                    .colors = colors,
                }
            );

            draw_commands_submit(
                canvas_stack_peek(canvasStack).drawCommands,
                uniformCache,
                scope,
                getCurrentShader(state),
                state.blendMode,
                whiteTexture->getRendererId()
            );
        }
    }

    void noFill() { peekState().isFillDisabled = true; }
    void fill(int grey, int alpha) { fill(color(grey, grey, grey, alpha)); }
    void fill(int red, int green, int blue, int alpha) { fill(color(red, green, blue, alpha)); }
    void fill(color_t color)
    {
        RenderState& state = peekState();
        state.fillColor = color;
        state.isFillDisabled = false;
    }

    void noStroke() { peekState().isStrokeDisabled = true; }
    void stroke(int grey, int alpha) { stroke(color(grey, grey, grey, alpha)); }
    void stroke(int red, int green, int blue, int alpha) { stroke(color(red, green, blue, alpha)); }
    void stroke(color_t color)
    {
        RenderState& state = peekState();
        state.strokeColor = color;
        state.isStrokeDisabled = false;
    }

    void strokeWeight(float strokeWeight) { peekState().strokeWeight = strokeWeight; }
    void strokeCap(StrokeCap strokeCap) { peekState().strokeCap = strokeCap; }
    void strokeJoin(StrokeJoin strokeJoin) { peekState().strokeJoin = strokeJoin; }
    void strokePattern(std::span<const float> pattern) { peekState().strokePatternSegments = std::vector<float>(pattern.begin(), pattern.end()); }
    void strokePatternOffset(float offset) { peekState().strokePatternOffset = offset; }
    void miterLimit(float miterLimit) { peekState().miterLimit = miterLimit; }
    void roundJoinThreshold(float angleThreshold) { peekState().roundJoinThreshold = angleThreshold; }
    void blendMode(BlendMode blendMode) { peekState().blendMode = blendMode; }
    void curveTightness(float tightness) { peekState().curveTightness = tightness; }

    void curveDetail(uint32_t detail)
    {
        detail = std::max(detail, 1u);

        RenderState& state = peekState();
        state.curveDetail = detail;
        state.invCurveDetail = 1.0f / static_cast<float>(detail);
    }

    void bezierDetail(uint32_t detail)
    {
        detail = std::max(detail, 1u);

        RenderState& state = peekState();
        state.bezierDetail = detail;
        state.invBezierDetail = 1.0f / static_cast<float>(detail);
    }

    void beginShape()
    {
    }

    // fillType  controls how the fill is tessellated into triangles.
    // strokeType controls how the outline is stroked — can differ from fillType
    // (e.g. fill a triangleFan but stroke only the outer lineLoop).
    void endShapeImpl(ShapeType fillType, ShapeType strokeType, std::optional<ColorStyle> fillStyle, std::optional<ColorStyle> strokeStyle, bool close)
    {
        const RenderState& state = peekState();

        flushIfNeeded();
        DrawScope scope = draw_buffer_get_scope(drawBuffer);

        if (fillStyle.has_value()) {
            const PathPoints points = linepath->buildDrawPoints(fillStyle.value());

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
            const PathPoints points = linepath->buildDrawPoints(strokeStyle.value());

            switch (strokeType) {
                case ShapeType::lines: stroke_lines(scope, points, state.strokeWeight, state.strokeCap, state.miterLimit, state.roundJoinThreshold); break;
                case ShapeType::lineStrip: stroke_line_strip(scope, points, state.strokeWeight, state.strokeCap, state.strokeJoin, state.miterLimit, state.roundJoinThreshold); break;
                case ShapeType::lineLoop: stroke_line_loop(scope, points, state.strokeWeight, state.strokeJoin, state.miterLimit, state.roundJoinThreshold); break;
                case ShapeType::triangles: stroke_triangles(scope, points, state.strokeWeight, state.strokeCap, state.strokeJoin, state.miterLimit, state.roundJoinThreshold); break;
                case ShapeType::triangleStrip: stroke_triangle_strip(scope, points, state.strokeWeight, state.strokeCap, state.strokeJoin, state.miterLimit, state.roundJoinThreshold); break;
                case ShapeType::triangleFan: stroke_triangle_fan(scope, points, state.strokeWeight, state.strokeCap, state.strokeJoin, state.miterLimit, state.roundJoinThreshold); break;
                case ShapeType::quads: stroke_quads(scope, points, state.strokeWeight, state.strokeCap, state.strokeJoin, state.miterLimit, state.roundJoinThreshold); break;
                case ShapeType::quadStrip: stroke_quad_strip(scope, points, state.strokeWeight, state.strokeCap, state.strokeJoin, state.miterLimit, state.roundJoinThreshold); break;
                case ShapeType::polygon: stroke_polygon(scope, points, state.strokeWeight, state.strokeCap, state.strokeJoin, state.miterLimit, state.roundJoinThreshold, close); break;
            }
        }

        draw_commands_submit(canvas_stack_peek(canvasStack).drawCommands, uniformCache, scope, getCurrentShader(state), state.blendMode, whiteTexture->getRendererId());

        linepath->clear();
        curveVertexCount = 0;
    }

    void endShape(ShapeType type, bool close)
    {
        const RenderState& state = peekState();
        const std::optional<ColorStyle> fillStyle = state.isFillDisabled ? std::nullopt : std::make_optional(ColorStyle::fill);
        const std::optional<ColorStyle> strokeStyle = state.isStrokeDisabled ? std::nullopt : std::make_optional(ColorStyle::stroke);
        endShapeImpl(type, type, fillStyle, strokeStyle, close);
    }

    void vertex(float x, float y)
    {
        vertex(x, y, 0.0f, 0.0f);
    }

    void vertex(float x, float y, float u, float v)
    {
        const RenderState& state = peekState();
        const float2 transformed = transformPoint(peekMatrix(), {x, y});
        linepath->vertex(transformed.x, transformed.y, u, v, state.fillColor, state.strokeColor);
    }

    void curveVertex(float x, float y)
    {
        const RenderState& state = peekState();
        curveVertexPositions[curveVertexCount++] = {x, y};

        if (curveVertexCount >= 4) {
            const RenderState& state = peekState();
            const float alpha = (1.0f - state.curveTightness) * 0.5f;

            for (size_t i = 0; i < curveVertexPositions.size() - 3; ++i) {
                auto [x1, y1] = curveVertexPositions[i];
                auto [x2, y2] = curveVertexPositions[i + 1];
                auto [x3, y3] = curveVertexPositions[i + 2];
                auto [x4, y4] = curveVertexPositions[i + 3];

                for (size_t j = 0; j <= state.curveDetail; ++j) {
                    float t = static_cast<float>(j) * state.invCurveDetail;
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

    void shader(std::shared_ptr<Shader> shader) { peekState().shader = std::move(shader); }
    void noShader() { peekState().shader.reset(); }
    void setUniform(const std::string& name, const UniformVariable& variable)
    {
        const RenderState& state = peekState();
        if (state.shader != nullptr) {
            setUniform(state.shader, name, variable);
        }
    }

    void setUniform(std::shared_ptr<Shader> shader, const std::string& name, const UniformVariable& variable)
    {
        ShaderUniformCache& shaderCache = uniform_cache_get_shader_cache(uniformCache, shader.get());
        shader_uniform_cache_insert_or_update(shaderCache, name, variable);
    }

    TextMetrics measureText(std::string_view text)
    {
        return measureText(text, getCurrentFont(peekState()).get(), peekState().textSize, 1.0f);
    }

    TextMetrics measureText(std::string_view text, Font* font, float textSize, float scale)
    {
        if (text.empty()) {
            return TextMetrics {
                .width = 0.0f,
                .totalHeight = 0.0f,
                .ascender = 0.0f,
                .descender = 0.0f,
                .lineCount = 0,
            };
        }

        const float lineHeight = font->getLineHeight(static_cast<int>(textSize)) * scale;

        float maxWidth = 0.0f;
        float lineWidth = 0.0f;
        float ascender = 0.0f;
        float descender = 0.0f;
        uint32_t lineCount = 1;

        char32_t prevCodepoint = 0;
        for (size_t i = 0; i < text.length();) {
            const char32_t codepoint = utf8NextCodepoint(text, i);

            if (codepoint == U'\n') {
                maxWidth = std::max(maxWidth, lineWidth);
                lineWidth = 0.0f;
                lineCount++;
                prevCodepoint = 0;
                continue;
            }

            if (codepoint == U' ') {
                if (const Glyph* spaceGlyph = font->getGlyph(U' ', static_cast<int>(textSize))) {
                    lineWidth += spaceGlyph->advance.x * scale;
                }
                prevCodepoint = U' ';
                continue;
            }

            if (prevCodepoint != 0) {
                lineWidth += font->getKerning(prevCodepoint, codepoint, static_cast<int>(textSize)) * scale;
            }

            if (const Glyph* glyph = font->getGlyph(codepoint, static_cast<int>(textSize))) {
                lineWidth += glyph->advance.x * scale;
                if (lineCount == 1) {
                    ascender = std::max(ascender, glyph->bearing.y * scale);
                }
                descender = std::max(descender, (glyph->size.y - glyph->bearing.y) * scale);
            }

            prevCodepoint = codepoint;
        }

        maxWidth = std::max(maxWidth, lineWidth);
        const float totalHeight = ascender + (lineHeight * (lineCount - 1)) + descender;

        return TextMetrics {
            .width = maxWidth,
            .totalHeight = totalHeight,
            .ascender = ascender,
            .descender = descender,
            .lineCount = lineCount,
        };
    }

    void textAlign(HorizontalTextAlign horizontalAlign, VerticalTextAlign verticalAlign)
    {
        RenderState& state = peekState();
        state.horizontalTextAlign = horizontalAlign;
        state.verticalTextAlign = verticalAlign;
    }

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

        static constexpr float HALF_PI = std::numbers::pi_v<float> * 0.5f;

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

        const RenderState& state = peekState();

        // Fill: triangleFan from the centre – rounded rect is always convex, so no libtess2 needed.
        if (!state.isFillDisabled) {
            beginShape();
            vertex(left + width * 0.5f, top + height * 0.5f); // fan centre
            emitRim();
            vertex(corners[0].cx + corners[0].rx, corners[0].cy); // re-emit first rim vertex to close the fan
            endShapeImpl(ShapeType::triangleFan, ShapeType::triangleFan, ColorStyle::fill, std::nullopt, false);
        }

        // Stroke: lineLoop around the rim only (no internal fan edges).
        if (!state.isStrokeDisabled) {
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
        const RenderState& state = peekState();
        const size_t segmentCount = computeCircleSegmentCount(2.0f * std::numbers::pi_v<float>, std::max(width, height) * 0.5f);

        // Fill: centre vertex + closed rim → triangleFan (no libtess2, O(n) vertices)
        if (!state.isFillDisabled) {
            beginShape();
            vertex(centerX, centerY);
            for (size_t i = 0; i <= segmentCount; ++i) {
                float angle = 2.0f * std::numbers::pi_v<float> / static_cast<float>(segmentCount) * static_cast<float>(i);
                vertex(centerX + std::cos(angle) * width * 0.5f, centerY + std::sin(angle) * height * 0.5f);
            }
            endShapeImpl(ShapeType::triangleFan, ShapeType::triangleFan, ColorStyle::fill, std::nullopt, false);
        }

        // Stroke: rim only → lineLoop, so only the outer edge is stroked, not the internal fan edges
        if (!state.isStrokeDisabled) {
            beginShape();
            for (size_t i = 0; i < segmentCount; ++i) {
                float angle = 2.0f * std::numbers::pi_v<float> / static_cast<float>(segmentCount) * static_cast<float>(i);
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
        if (peekState().isStrokeDisabled) return;

        const RenderState& state = peekState();
        const size_t segmentCount = computeCircleSegmentCount(2.0f * std::numbers::pi_v<float>, state.strokeWeight * 0.5f);

        // Fill a disc with the stroke color as a triangleFan — no libtess2, no outline edges
        beginShape();
        vertex(x, y);
        for (size_t i = 0; i <= segmentCount; ++i) {
            float angle = 2.0f * std::numbers::pi_v<float> / static_cast<float>(segmentCount) * static_cast<float>(i);
            vertex(x + std::cos(angle) * state.strokeWeight * 0.5f, y + std::sin(angle) * state.strokeWeight * 0.5f);
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
        const bool clockwise = sweepAngle < 0.0f;
        const float radius = std::max(width, height) * 0.5f;
        const size_t segmentCount = computeCircleSegmentCount(sweepAngle, radius);

        beginShape();

        if (arcMode == ArcMode::pie) {
            vertex(centerX, centerY);
        }

        for (size_t i = 0; i <= segmentCount; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(segmentCount);
            float angle = startAngle + (clockwise ? -1.0f : 1.0f) * t * sweepAngle;
            float x = centerX + std::cos(angle) * width * 0.5f;
            float y = centerY + std::sin(angle) * height * 0.5f;
            vertex(x, y);
        }

        endShape(ShapeType::polygon, arcMode != ArcMode::open);
    }

    void bezier(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
    {
        const RenderState& state = peekState();

        beginShape();
        for (size_t i = 0; i <= state.bezierDetail; ++i) {
            float t = static_cast<float>(i) * state.invBezierDetail;

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
        const RenderState& state = peekState();

        float alpha = (1.0f - state.curveTightness) * 0.5f;

        beginShape();
        for (size_t i = 0; i <= state.curveDetail; ++i) {
            float t = static_cast<float>(i) * state.invCurveDetail;
            float t2 = t * t;
            float t3 = t2 * t;

            float bx = alpha * ((-x1 + 3 * x2 - 3 * x3 + x4) * t3 + (2 * x1 - 5 * x2 + 4 * x3 - x4) * t2 + (-x1 + x3) * t) + x2;
            float by = alpha * ((-y1 + 3 * y2 - 3 * y3 + y4) * t3 + (2 * y1 - 5 * y2 + 4 * y3 - y4) * t2 + (-y1 + y3) * t) + y2;

            vertex(bx, by);
        }
        endShapeImpl(ShapeType::lineStrip, ShapeType::lineStrip, std::nullopt, ColorStyle::stroke, false);
    }

    void tint(int grey, int alpha) { tint(color(grey, grey, grey, alpha)); }
    void tint(int red, int green, int blue, int alpha) { tint(color(red, green, blue, alpha)); }
    void tint(color_t color) { peekState().tintColor = color; }
    void noTint() { peekState().tintColor = color(255, 255, 255); }
    void image(uint32_t textureId, float left, float top, float width, float height)
    {
        RenderState& state = peekState();
        const std::array<float2, 4> positions = {
            transformPoint(peekMatrix(), {left, top}),
            transformPoint(peekMatrix(), {left + width, top}),
            transformPoint(peekMatrix(), {left + width, top + height}),
            transformPoint(peekMatrix(), {left, top + height}),
        };
        const std::array<float2, 4> texcoords = {
            float2 {0.0f, 1.0f},
            float2 {1.0f, 1.0f},
            float2 {1.0f, 0.0f},
            float2 {0.0f, 0.0f}
        };
        const std::array<color_t, 4> colors = {
            state.tintColor,
            state.tintColor,
            state.tintColor,
            state.tintColor
        };

        {
            flushIfNeeded();
            DrawScope scope = draw_buffer_get_scope(drawBuffer);

            tesselate_quads(
                scope,
                PathPoints {
                    .size = 4,
                    .positions = positions,
                    .texcoords = texcoords,
                    .colors = colors,
                }
            );

            draw_commands_submit(canvas_stack_peek(canvasStack).drawCommands, uniformCache, scope, getCurrentShader(state), state.blendMode, textureId);
        }
    }

    void textFont(std::shared_ptr<Font> font) { peekState().font = font; }
    void noTextFont() { peekState().font.reset(); }
    void textSize(float size) { peekState().textSize = size; }

    void text(std::string_view text, float x, float y)
    {
        if (text.empty()) return;

        RenderState& state = peekState();
        if (state.isFillDisabled) return; // fill color is the text color

        Font* font = getCurrentFont(state).get();

        TextMetrics metrics = measureText(text, font, state.textSize, 1.0f);

        // The horizontal start position for every line (alignment is against the full text width).
        float lineStartX;
        switch (state.horizontalTextAlign) {
            case HorizontalTextAlign::left: lineStartX = x; break;
            case HorizontalTextAlign::center: lineStartX = x - metrics.width * 0.5f; break;
            case HorizontalTextAlign::right: lineStartX = x - metrics.width; break;
        }

        float py;
        switch (state.verticalTextAlign) {
            case VerticalTextAlign::top: py = y + metrics.ascender; break;
            case VerticalTextAlign::center: py = y + metrics.ascender - metrics.totalHeight * 0.5f; break;
            case VerticalTextAlign::baseline: py = y; break;
            case VerticalTextAlign::bottom: py = y - metrics.totalHeight + metrics.ascender; break;
        }

        const Glyph* spaceGlyph = font->getGlyph(U' ', static_cast<int>(state.textSize));
        float px = lineStartX;
        bool isFirstCharOnLine = true; // tracks whether we need to apply bearing correction for this line

        char32_t prevCh = 0;
        for (size_t i = 0; i < text.length();) {
            const char32_t ch = utf8NextCodepoint(text, i);

            if (ch == U'\n') {
                px = lineStartX; // reset to alignment-adjusted start, not raw x
                py += font->getLineHeight(state.textSize);
                isFirstCharOnLine = true;
                prevCh = 0;
                continue;
            }

            if (ch == U' ') {
                if (spaceGlyph != nullptr) {
                    px += spaceGlyph->advance.x;
                    py += spaceGlyph->advance.y;
                }
                prevCh = U' ';
                continue;
            }

            const Glyph* glyph = font->getGlyph(ch, static_cast<int>(state.textSize));
            if (glyph == nullptr) {
                prevCh = ch;
                continue;
            }

            if (isFirstCharOnLine) {
                // Shift the pen left so the first glyph's visual left edge starts exactly at lineStartX.
                px -= glyph->bearing.x;
                isFirstCharOnLine = false;
            } else {
                px += font->getKerning(prevCh, ch, static_cast<int>(state.textSize));
            }

            const float left = px + glyph->bearing.x;
            const float top = py - glyph->bearing.y;
            const float right = left + glyph->size.x;
            const float bottom = top + glyph->size.y;

            const std::array<float2, 4> positions = {
                transformPoint(peekMatrix(), {left, top}),
                transformPoint(peekMatrix(), {right, top}),
                transformPoint(peekMatrix(), {right, bottom}),
                transformPoint(peekMatrix(), {left, bottom}),
            };
            const rect2f uvRect = glyph->uvRect;
            const std::array<float2, 4> texcoords = {
                float2 {uvRect.left, uvRect.top},
                float2 {uvRect.left + uvRect.width, uvRect.top},
                float2 {uvRect.left + uvRect.width, uvRect.top + uvRect.height},
                float2 {uvRect.left, uvRect.top + uvRect.height}
            };
            const std::array<color_t, 4> colors = {
                state.fillColor,
                state.fillColor,
                state.fillColor,
                state.fillColor
            };

            {
                flushIfNeeded();
                DrawScope scope = draw_buffer_get_scope(drawBuffer);
                tesselate_quads(
                    scope,
                    PathPoints {
                        .size = 4,
                        .positions = positions,
                        .texcoords = texcoords,
                        .colors = colors,
                    }
                );

                std::shared_ptr<Shader> shader = state.shader != nullptr ? state.shader : textShader;
                draw_commands_submit(canvas_stack_peek(canvasStack).drawCommands, uniformCache, scope, std::move(shader), state.blendMode, font->getGlyphPageTextureId(glyph->pageIndex));
            }

            px += glyph->advance.x;
            py += glyph->advance.y;
            prevCh = ch;
        }
    }
} // namespace p5

namespace p5
{
    int mouseX = 0;
    int mouseY = 0;
    int width = 0;
    int height = 0;
    int frameCount = 0;
    float fps = 0.0f;
    float deltaTime = 0.0f;

    inline static float s_targetFrameTime = 0.0f;
    inline static auto s_appStartTime = std::chrono::steady_clock::now();
    inline static bool s_isAppPaused = false;
    inline static bool s_isCloseRequested = false;
    inline static int s_exitCode = 0;

    void setWindowSize(int w, int h)
    {
        window_set_size(appWindow, w, h);
        width = w;
        height = h;
    }
    void setWindowTitle(std::string_view title) { window_set_title(appWindow, title); }
    void setWindowResizable(bool resizable) { window_set_resizable(appWindow, resizable); }
    int getWindowWidth() { return window_logical_width(appWindow); }
    int getWindowHeight() { return window_logical_height(appWindow); }

    void frameRate(float targetFps) { s_targetFrameTime = (targetFps > 0.0f) ? (1.0f / targetFps) : 0.0f; }
    void loop() { s_isAppPaused = false; }
    void noLoop() { s_isAppPaused = true; }
    bool isLooping() { return !s_isAppPaused; }
    void quit() { s_isCloseRequested = true; }

    void quit(int exitCode)
    {
        p5::exitCode(exitCode);
        quit();
    }

    void exitCode(int code) { s_exitCode = code; }

    float millis()
    {
        return std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - s_appStartTime).count();
    }

} // namespace p5

int main()
{
    using namespace p5;

    static std::unique_ptr sketch = createSketch();

    appWindow = window_create(800, 600, "p5", [&](const WindowEvent& e) {
        if (e.type == EventType::close) {
            quit();
        }
        if (e.type == EventType::mouseMove) {
            mouseX = e.mouseMove.x;
            mouseY = e.mouseMove.y;
        }
        if (e.type == EventType::windowResize) {
            width = e.windowResize.width;
            height = e.windowResize.height;
        }
        sketch->event(e);
    });

    width = window_logical_width(appWindow);
    height = window_logical_height(appWindow);

    static constexpr size_t MAX_VERTICES = 65536;
    static constexpr size_t MAX_INDICES = MAX_VERTICES * 3;
    static constexpr uint8_t whitePixel[] = {255, 255, 255, 255};

    renderer = renderer_create(MAX_VERTICES, MAX_INDICES);
    whiteTexture = loadTexture(1, 1, whitePixel);
    drawBuffer = draw_buffer_create(MAX_VERTICES, MAX_INDICES);
    linepath = std::make_unique<LinePathBuilder>();
    defaultShader = createDefaultShader();
    textShader = createTextShader();
    defaultFont = loadFont({DejaVuSans_ttf, DejaVuSans_ttf_len});
    uniformCache = uniform_cache_create();
    defaultFramebuffer = window_default_framebuffer(appWindow);

    {
        renderer_begin_frame(renderer);
        pushCanvas(defaultFramebuffer);
        sketch->setup();
        popCanvas();
        renderer_end_frame(renderer, drawBuffer);
    }

    window_show(appWindow);

    FpsCounter fpsCounter;
    auto lastFrameTime = Clock::now();

    while (not s_isCloseRequested) {
        const TimePoint frameStart = Clock::now();
        deltaTime = std::chrono::duration<float>(frameStart - lastFrameTime).count();
        // Cap deltaTime so frame drops don't cause large physics jumps
        deltaTime = std::min(deltaTime, 1.0f / 20.0f);
        lastFrameTime = frameStart;

        window_poll_events(appWindow);

        if (not s_isAppPaused) {
            renderer_begin_frame(renderer);
            pushCanvas(defaultFramebuffer);
            sketch->draw();
            popCanvas();
            renderer_end_frame(renderer, drawBuffer);

            window_swap_buffers(appWindow);
            frameCount++;

            fpsCounter.tick();
            fps = fpsCounter.fps();
        }

        // FPS limiter: precise sleep for remainder of target frame duration
        if (s_targetFrameTime > 0.0f) {
            const auto targetDuration = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<float>(s_targetFrameTime));
            precise_sleep_until(frameStart + targetDuration);
        }
    }

    sketch->destroy();
    sketch.reset();
    window_destroy(appWindow);

    return s_exitCode;
}
