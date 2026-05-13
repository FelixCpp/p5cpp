#include "p5.hpp"
#include "drawcall.hpp"
#include "renderpassstack.hpp"
#include "rendering.hpp"
#include "linepath.hpp"
#include "renderstate.hpp"
#include "shader.hpp"
#include "tess.hpp"
#include "stroker.hpp"
#include "dejavusans.hpp"
#include <cassert>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <iostream>
#include <array>
#include <numbers>
#include <algorithm>

namespace p5
{
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
    void error(std::string_view message) { std::cout << "[ERROR]: " << message << std::endl; }
} // namespace p5

namespace p5
{
    struct Window
    {
        GLFWwindow* handle;
        int framebufferWidth;
        int framebufferHeight;
        int windowWidth;
        int windowHeight;
    };

    inline static size_t curveVertexCount;
    inline static std::array<float2, 4> curveVertexPositions;

    inline static std::unique_ptr<LinePathBuilder> linepath;
    inline static std::unique_ptr<Tesselator> fanTesselator;
    inline static std::unique_ptr<Tesselator> concaveTesselator;
    inline static std::shared_ptr<Shader> defaultShader;
    inline static std::shared_ptr<Shader> textShader;
    inline static std::shared_ptr<Font> defaultFont;
    inline static std::shared_ptr<Canvas> defaultCanvas;
    inline static RenderStateStack renderStates;
    inline static std::unique_ptr<Texture> whiteTexture;
    inline static RenderPassStack renderPassStack;
    inline static Renderer renderer;
    inline static DrawBuffer drawBuffer;

    inline Window window;

    inline RenderState& peekState() { return render_state_stack_peek(renderStates); }
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

    void pushCanvas(std::shared_ptr<Canvas> canvas) { render_passes_push(renderPassStack, std::move(canvas)); }
    void popCanvas() { render_passes_pop(renderPassStack); }

    void pushState() { render_state_stack_push(renderStates, peekState()); }
    void popState() { render_state_stack_pop(renderStates); }

    void pushMatrix() { matrix_stack_push(peekState().metrics, matrix_stack_peek(peekState().metrics)); }
    void popMatrix() { matrix_stack_pop(peekState().metrics); }
    void resetMatrix() { matrix_stack_reset(peekState().metrics); }
    void applyMatrix(const matrix4x4& matrix) { matrix_stack_apply(peekState().metrics, matrix); }
    void setMatrix(const matrix4x4& matrix) { matrix_stack_set(peekState().metrics, matrix); }
    matrix4x4& peekMatrix() { return matrix_stack_peek(peekState().metrics); }
    void translate(float x, float y) { applyMatrix(translation(x, y)); }
    void scale(float x, float y) { applyMatrix(scaling(x, y)); }
    void rotate(float angle) { applyMatrix(rotation(angle)); }

    void background(int grey, int alpha) { background(color(grey, grey, grey, alpha)); }
    void background(int red, int green, int blue, int alpha) { background(color(red, green, blue, alpha)); }
    void background(color_t color)
    {
        const RenderState& state = peekState();
        const uint2 size = render_passes_get_active_canvas_size(renderPassStack);
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
        const std::array<color_t, 4> colors = {color, color, color, color};

        DrawScope scope = draw_buffer_get_scope(drawBuffer);
        fanTesselator->tesselate(
            scope,
            PathPoints {
                .size = 4,
                .positions = positions,
                .texcoords = texcoords,
                .colors = colors,
            }
        );

        draw_calls_submit(scope, render_passes_peek(renderPassStack).drawCalls, getCurrentShader(state), state.blendMode, whiteTexture->getRendererId());
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

    enum class ShapeType {
        convex,
        concave,
    };

    void endShapeImpl(bool shouldClose, FillStyle fillStyle, FillStyle strokeStyle, ShapeType type)
    {
        const RenderState& state = peekState();

        {
            DrawScope scope = draw_buffer_get_scope(drawBuffer);

            if (fillStyle != FillStyle::none) {
                switch (type) {
                    case ShapeType::convex: fanTesselator->tesselate(scope, linepath->buildDrawPoints(fillStyle)); break;
                    case ShapeType::concave: concaveTesselator->tesselate(scope, linepath->buildDrawPoints(fillStyle)); break;
                }
            }

            if (strokeStyle != FillStyle::none) {
                generateSolidStroke(scope, linepath->buildDrawPoints(strokeStyle), state.strokeWeight, state.strokeCap, state.strokeJoin, state.miterLimit, state.roundJoinThreshold, shouldClose);
            }

            draw_calls_submit(scope, render_passes_peek(renderPassStack).drawCalls, getCurrentShader(state), state.blendMode, whiteTexture->getRendererId());
        }

        linepath->clear();
        curveVertexCount = 0;
    }

    void endShape(const bool close, ShapeType type)
    {
        const RenderState& state = peekState();
        const FillStyle fillStyle = state.isFillDisabled ? FillStyle::none : FillStyle::fill;
        const FillStyle strokeStyle = state.isStrokeDisabled ? FillStyle::none : FillStyle::stroke;
        endShapeImpl(close, fillStyle, strokeStyle, type);
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
        curveVertexPositions[curveVertexCount++] = transformPoint(peekMatrix(), {x, y});

        if (curveVertexCount >= 4) {
            const RenderState& state = peekState();
            float alpha = (1.0f - state.curveTightness) * 0.5f;

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
    void setUniform(std::string_view name, float x)
    {
        if (std::shared_ptr<Shader> currentShader = getCurrentShader(peekState()); currentShader != nullptr) {
            glProgramUniform1f(currentShader->getRendererId(), currentShader->getUniformLocation(name), x);
        }
    }
    void setUniform(std::string_view name, float x, float y)
    {
        if (std::shared_ptr<Shader> currentShader = getCurrentShader(peekState()); currentShader != nullptr) {
            glProgramUniform2f(currentShader->getRendererId(), currentShader->getUniformLocation(name), x, y);
        }
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

        for (size_t i = 0; i < text.length(); ++i) {
            const char32_t codepoint = static_cast<char32_t>(text[i]);

            if (codepoint == U'\n') {
                maxWidth = std::max(maxWidth, lineWidth);
                lineWidth = 0.0f;
                lineCount++;
                continue;
            }

            if (codepoint == U' ') {
                if (const Glyph* spaceGlyph = font->getGlyph(U' ', static_cast<int>(textSize))) {
                    lineWidth += spaceGlyph->advance.x * scale;
                }

                continue;
            }

            const bool hasPrevious = i > 0 && text[i - 1] != '\n';
            if (hasPrevious) {
                const char32_t previousCodepoint = static_cast<char32_t>(text[i - 1]);
                lineWidth += font->getKerning(previousCodepoint, codepoint, static_cast<int>(textSize)) * scale;
            }

            if (const Glyph* glyph = font->getGlyph(codepoint, static_cast<int>(textSize))) {
                lineWidth += glyph->advance.x * scale;
                if (lineCount == 1) {
                    ascender = std::max(ascender, glyph->bearing.y * scale);
                }

                descender = std::min(descender, (glyph->size.y - glyph->bearing.y) * scale);
            }
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
        endShape(true, ShapeType::convex);
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
            float cx, cy;
            float rx, ry;
            float startAngle;
        };

        Corner corners[4] = {
            {left + width - bottomRightX, top + height - bottomRightY, bottomRightX, bottomRightY, 0.0f},                 // unten rechts
            {left + bottomLeftX, top + height - bottomLeftY, bottomLeftX, bottomLeftY, std::numbers::pi_v<float> * 0.5f}, // unten links
            {left + topLeftX, top + topLeftY, topLeftX, topLeftY, std::numbers::pi_v<float>},                             // oben links
            {left + width - topRightX, top + topRightY, topRightX, topRightY, std::numbers::pi_v<float> * 1.5f},          // oben rechts
        };

        constexpr size_t cornerSegments = 8;

        beginShape();

        for (const auto& corner : corners) {
            for (size_t i = 0; i <= cornerSegments; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(cornerSegments);
                float angle = corner.startAngle + t * std::numbers::pi_v<float> * 0.5f;
                float vx = corner.cx + std::cos(angle) * corner.rx;
                float vy = corner.cy + std::sin(angle) * corner.ry;
                vertex(vx, vy);
            }
        }

        endShape(true, ShapeType::convex);
    }

    void square(float left, float top, float size)
    {
        rect(left, top, size, size);
    }

    void ellipse(float centerX, float centerY, float width, float height)
    {
        const size_t segmentCount = computeCircleSegmentCount(2.0f * std::numbers::pi_v<float>, std::max(width, height) * 0.5f);

        beginShape();
        for (size_t i = 0; i < segmentCount; ++i) {
            float angle = 2.0f * std::numbers::pi_v<float> / static_cast<float>(segmentCount) * static_cast<float>(i);
            float x = centerX + std::cos(angle) * width * 0.5f;
            float y = centerY + std::sin(angle) * height * 0.5f;
            vertex(x, y);
        }
        endShape(true, ShapeType::convex);
    }

    void circle(float centerX, float centerY, float size)
    {
        ellipse(centerX, centerY, size, size);
    }

    void point(float x, float y)
    {
        const RenderState& state = peekState();
        const size_t segmentCount = computeCircleSegmentCount(2.0f * std::numbers::pi_v<float>, state.strokeWeight * 0.5f);

        beginShape();
        for (size_t i = 0; i < segmentCount; ++i) {
            float angle = 2.0f * std::numbers::pi_v<float> / static_cast<float>(segmentCount) * static_cast<float>(i);
            float px = x + std::cos(angle) * state.strokeWeight * 0.5f;
            float py = y + std::sin(angle) * state.strokeWeight * 0.5f;
            vertex(px, py);
        }
        endShapeImpl(false, FillStyle::stroke, FillStyle::none, ShapeType::convex);
    }

    void triangle(float x1, float y1, float x2, float y2, float x3, float y3)
    {
        beginShape();
        vertex(x1, y1);
        vertex(x2, y2);
        vertex(x3, y3);
        endShape(true, ShapeType::convex);
    }

    void line(float x1, float y1, float x2, float y2)
    {
        const RenderState& state = peekState();
        float halfStrokeWeight = state.strokeWeight * 0.5f;

        const float dx = x2 - x1;
        const float dy = y2 - y1;
        const float len = std::sqrt(dx * dx + dy * dy);

        if (len == 0.0f) return;

        const float ndx = dx / len;
        const float ndy = dy / len;

        if (state.strokeCap == StrokeCap::square) {
            x1 -= ndx * halfStrokeWeight;
            y1 -= ndy * halfStrokeWeight;
            x2 += ndx * halfStrokeWeight;
            y2 += ndy * halfStrokeWeight;
        }

        const float px = -ndy;
        const float py = ndx;

        const float ox = px * halfStrokeWeight;
        const float oy = py * halfStrokeWeight;

        beginShape();
        if (state.strokeCap != StrokeCap::round) {
            vertex(x1 + ox, y1 + oy);
            vertex(x2 + ox, y2 + oy);
            vertex(x2 - ox, y2 - oy);
            vertex(x1 - ox, y1 - oy);
        } else {
            const size_t segments = 8;
            float baseAngle = std::atan2(dy, dx);

            for (size_t i = 0; i <= segments; ++i) {
                float t = static_cast<float>(i) / segments;
                float a = baseAngle + std::numbers::pi_v<float> * 0.5f + t * std::numbers::pi_v<float>;

                float x = x1 + std::cos(a) * halfStrokeWeight;
                float y = y1 + std::sin(a) * halfStrokeWeight;

                vertex(x, y);
            }

            for (size_t i = 0; i <= segments; ++i) {
                float t = static_cast<float>(i) / segments;
                float a = baseAngle - std::numbers::pi_v<float> * 0.5f + t * std::numbers::pi_v<float>;

                float x = x2 + std::cos(a) * halfStrokeWeight;
                float y = y2 + std::sin(a) * halfStrokeWeight;

                vertex(x, y);
            }
        }
        endShapeImpl(false, FillStyle::stroke, FillStyle::none, ShapeType::convex); // TODO: Is the ShapeType correct?
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

        endShape(arcMode != ArcMode::open, ShapeType::convex);
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
        endShapeImpl(false, FillStyle::none, FillStyle::stroke, ShapeType::convex);
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
        endShapeImpl(false, FillStyle::none, FillStyle::stroke, ShapeType::convex);
    }

    void tint(int grey, int alpha) { tint(color(grey, grey, grey, alpha)); }
    void tint(int red, int green, int blue, int alpha) { tint(color(red, green, blue, alpha)); }
    void tint(color_t color) { peekState().tintColor = color; }
    void noTint() { peekState().tintColor = color(255, 255, 255); }
    void image(uint32_t textureId, float left, float top, float width, float height)
    {
        const RenderState& state = peekState();
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
            DrawScope scope = draw_buffer_get_scope(drawBuffer);
            fanTesselator->tesselate(
                scope,
                PathPoints {
                    .size = 4,
                    .positions = positions,
                    .texcoords = texcoords,
                    .colors = colors,
                }
            );

            draw_calls_submit(scope, render_passes_peek(renderPassStack).drawCalls, getCurrentShader(state), state.blendMode, textureId);
        }
    }

    void textFont(std::shared_ptr<Font> font)
    {
        peekState().font = font;
    }
    void noTextFont() { peekState().font.reset(); }
    void textSize(float size) { peekState().textSize = size; }

    void text(std::string_view text, float x, float y)
    {
        if (text.empty()) return;

        const RenderState& state = peekState();
        Font* font = getCurrentFont(state).get();

        TextMetrics metrics = measureText(text, font, state.textSize, 1.0f);

        float px;
        switch (state.horizontalTextAlign) {
            case HorizontalTextAlign::left: px = x; break;
            case HorizontalTextAlign::center: px = x - metrics.width * 0.5f; break;
            case HorizontalTextAlign::right: px = x - metrics.width; break;
        }

        float py;
        switch (state.verticalTextAlign) {
            case VerticalTextAlign::top: py = y + metrics.ascender; break;
            case VerticalTextAlign::center: py = y + metrics.ascender * 0.5f; break;
            case VerticalTextAlign::baseline: py = y; break;
            case VerticalTextAlign::bottom: py = y - metrics.descender; break;
        }

        if (const Glyph* firstGlyph = font->getGlyph(text.front(), state.textSize)) {
            px -= firstGlyph->bearing.x;
        }

        const Glyph* spaceGlyph = font->getGlyph(U' ', static_cast<int>(state.textSize));
        for (size_t i = 0; i < text.length(); ++i) {
            const char32_t ch = text[i];

            if (ch == U'\n') {
                px = x;
                py += font->getLineHeight(state.textSize);
                continue;
            }

            if (ch == U' ') {
                px += spaceGlyph->advance.x;
                py += spaceGlyph->advance.y;
                continue;
            }

            const Glyph* glyph = font->getGlyph(ch, static_cast<int>(state.textSize));
            if (glyph == nullptr) {
                continue;
            }

            const bool hasPrevious = i > 0 && text[i - 1] != '\n';
            if (hasPrevious) {
                const char32_t previousCh = text[i - 1];
                px += font->getKerning(previousCh, ch, static_cast<int>(state.textSize));
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
                DrawScope scope = draw_buffer_get_scope(drawBuffer);
                fanTesselator->tesselate(
                    scope,
                    PathPoints {
                        .size = 4,
                        .positions = positions,
                        .texcoords = texcoords,
                        .colors = colors,
                    }
                );

                std::shared_ptr<Shader> shader = state.shader != nullptr ? state.shader : textShader;
                draw_calls_submit(scope, render_passes_peek(renderPassStack).drawCalls, std::move(shader), state.blendMode, font->getGlyphPageTextureId(glyph->pageIndex));
            }

            px += glyph->advance.x;
            py += glyph->advance.y;
        }
    }
} // namespace p5

namespace p5
{
    int mouseX = 0;
    int mouseY = 0;

    matrix4x4 projectionMatrix;
} // namespace p5

int main()
{
    using namespace p5;

    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window.windowWidth = 800;
    window.windowHeight = 600;
    projectionMatrix = ortho(0.0f, static_cast<float>(window.windowHeight), static_cast<float>(window.windowWidth), 0.0f, -1.0f, 1.0f);

    window.handle = glfwCreateWindow(window.windowWidth, window.windowHeight, "p5", nullptr, nullptr);
    glfwMakeContextCurrent(window.handle);
    glfwSwapInterval(1);

    glfwSetWindowSizeCallback(window.handle, [](GLFWwindow* handle, int width, int height) {
        window.windowWidth = width;
        window.windowHeight = height;
        projectionMatrix = ortho(0.0f, static_cast<float>(window.windowHeight), static_cast<float>(window.windowWidth), 0.0f, -1.0f, 1.0f);
    });

    glfwSetFramebufferSizeCallback(window.handle, [](GLFWwindow* handle, int width, int height) {
        window.framebufferWidth = width;
        window.framebufferHeight = height;
    });

    glfwSetCursorPosCallback(window.handle, [](GLFWwindow* handle, double x, double y) {
        mouseX = static_cast<int>(x);
        mouseY = static_cast<int>(y);
    });

    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&glfwGetProcAddress));

    glEnable(GL_MULTISAMPLE);

    std::cout << glGetString(GL_VERSION) << std::endl;
    std::cout << glGetString(GL_VENDOR) << std::endl;
    std::cout << glGetString(GL_RENDERER) << std::endl;
    std::cout << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    glfwGetFramebufferSize(window.handle, &window.framebufferWidth, &window.framebufferHeight);

    static constexpr size_t MAX_VERTICES = 65536;
    static constexpr size_t MAX_INDICES = MAX_VERTICES * 3;
    static constexpr uint8_t whitePixel[] = {255, 255, 255, 255};

    renderer = renderer_create(MAX_VERTICES, MAX_INDICES);
    whiteTexture = loadTexture(1, 1, whitePixel);
    drawBuffer = draw_buffer_create(MAX_VERTICES, MAX_INDICES);
    fanTesselator = createFanTesselator();
    concaveTesselator = createConcaveTesselator();
    linepath = std::make_unique<LinePathBuilder>();
    defaultShader = createDefaultShader();
    textShader = createTextShader();
    defaultFont = loadFont({DejaVuSans_ttf, DejaVuSans_ttf_len});
    defaultCanvas = createCanvas(window.framebufferWidth, window.framebufferHeight);
    renderStates = render_state_stack_create();

    static std::unique_ptr sketch = createSketch();
    renderer_begin_frame(renderer);
    {
        render_passes_push(renderPassStack, defaultCanvas);
        {
            sketch->setup();
        }
        render_passes_pop(renderPassStack);
    }
    renderer_end_frame(renderer, renderPassStack, drawBuffer);

    glfwSetMouseButtonCallback(window.handle, [](GLFWwindow* handle, int button, int action, int) {
        if (action == GLFW_PRESS) {
            double mx, my;
            glfwGetCursorPos(handle, &mx, &my);
            sketch->mousePressed(mx, my);
        }
    });

    glfwShowWindow(window.handle);

    while (not glfwWindowShouldClose(window.handle)) {
        glfwPollEvents();
        glfwGetFramebufferSize(window.handle, &window.framebufferWidth, &window.framebufferHeight);

        renderer_begin_frame(renderer);
        {
            render_passes_push(renderPassStack, defaultCanvas);
            {
                sketch->draw();
            }
            render_passes_pop(renderPassStack);
        }
        renderer_end_frame(renderer, renderPassStack, drawBuffer);
        render_state_stack_clear(renderStates);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, defaultCanvas->getRendererId());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, defaultCanvas->getSize().x, defaultCanvas->getSize().y, 0, 0, window.framebufferWidth, window.framebufferHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::fprintf(stdout, "GL Error after blit: 0x%x\n", err);
            std::fflush(stdout);
        }

        glfwSwapBuffers(window.handle);
    }

    sketch->destroy();
    sketch.reset();
    glfwTerminate();

    return 0;
}
