#include "p5.hpp"
#include "render_state_stack.hpp"
#include "utf8_view.hpp"
#include "window.hpp"
#include "framebuffer.hpp"
#include "shader.hpp"
#include "renderer.hpp"
#include "linepath.hpp"
#include "render_state.hpp"
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

namespace p5
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
} // namespace p5

namespace p5
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
    struct AppState
    {
        int mouseX;
        int mouseY;
        int pmouseX;
        int pmouseY;

        int width;
        int height;

        int frameCount;
        int frameRate;

        float deltaTime;
        float globalTime;
    };

    inline static AppState appState;

    int getMouseX() { return appState.mouseX; }
    int getMouseY() { return appState.mouseY; }
    int getPMouseX() { return appState.pmouseX; }
    int getPMouseY() { return appState.pmouseY; }

    int getWidth() { return appState.width; }
    int getHeight() { return appState.height; }

    int getFrameCount() { return appState.frameCount; }
    int getFrameRate() { return appState.frameRate; }
    float getDeltaTime() { return appState.deltaTime; }
    float getGlobalTime() { return appState.globalTime; }

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
    inline static std::vector<std::shared_ptr<Framebuffer>> framebufferStack;
    inline static RenderStateStack renderStateStack;
    inline static Renderer renderer;

    inline static AppWindow* appWindow = nullptr;

    inline static std::vector<color_t> pixelScratch;
    inline static bool needsDefaultCanvasRecreation = false;
} // namespace p5

/// --------------------------------------------
///               RenderState Options
/// --------------------------------------------
namespace p5
{
    void pushState()
    {
        render_state_stack_push(renderStateStack, render_state_stack_peek(renderStateStack));
    }

    void popState()
    {
        render_state_stack_pop(renderStateStack);
    }

    void pushMatrix()
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        matrix_stack_push(renderState.metrics, matrix_stack_peek(renderState.metrics));
    }

    void popMatrix()
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        matrix_stack_pop(renderState.metrics);
    }

    void resetMatrix()
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        matrix_stack_set(renderState.metrics, matrix4x4::identity);
    }

    void applyMatrix(const matrix4x4& matrix)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        matrix_stack_apply(renderState.metrics, matrix);
    }

    void setMatrix(const matrix4x4& matrix)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        matrix_stack_set(renderState.metrics, matrix);
    }

    matrix4x4& peekMatrix()
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        return matrix_stack_peek(renderState.metrics);
    }

    void translate(float x, float y)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        matrix_stack_apply(renderState.metrics, translation(x, y));
    }

    void scale(float x, float y)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        matrix_stack_apply(renderState.metrics, scaling(x, y));
    }

    void rotate(float angle)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        matrix_stack_apply(renderState.metrics, rotation(angle));
    }

    void noFill()
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.isFillDisabled = true;
    }

    void fill(int grey, int alpha) { fill(rgba(grey, grey, grey, alpha)); }
    void fill(int red, int green, int blue, int alpha) { fill(rgba(red, green, blue, alpha)); }
    void fill(color_t color)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.fillColor = color;
        renderState.isFillDisabled = false;
    }

    void noStroke()
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.isStrokeDisabled = true;
    }

    void stroke(int grey, int alpha) { stroke(rgba(grey, grey, grey, alpha)); }
    void stroke(int red, int green, int blue, int alpha) { stroke(rgba(red, green, blue, alpha)); }
    void stroke(color_t color)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.strokeColor = color;
        renderState.isStrokeDisabled = false;
    }

    void strokeWeight(float strokeWeight)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.strokeWeight = strokeWeight;
    }

    void strokeCap(StrokeCap strokeCap)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.strokeCap = strokeCap;
    }

    void strokeJoin(StrokeJoin strokeJoin)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.strokeJoin = strokeJoin;
    }

    void miterLimit(float miterLimit)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.miterLimit = miterLimit;
    }

    void roundJoinThreshold(float angleThreshold)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.roundJoinThreshold = angleThreshold;
    }

    void blendMode(BlendMode blendMode)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.blendMode = blendMode;
    }

    void curveTightness(float tightness)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.curveTightness = tightness;
    }

    void curveDetail(uint32_t detail)
    {
        detail = std::max(detail, 1u);

        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.curveDetail = detail;
        renderState.invCurveDetail = 1.0f / static_cast<float>(detail);
    }

    void bezierDetail(uint32_t detail)
    {
        detail = std::max(detail, 1u);

        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.bezierDetail = detail;
        renderState.invBezierDetail = 1.0f / static_cast<float>(detail);
    }

    void shader(std::shared_ptr<Shader> shader)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.shader = std::move(shader);
    }

    void noShader()
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.shader.reset();
    }

    void setUniform(const std::string& name, const UniformVariable& variable)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        if (renderState.shader != nullptr) {
            setUniform(renderState.shader, name, variable);
        }
    }

    void setUniform(std::shared_ptr<Shader> shader, const std::string& name, const UniformVariable& variable)
    {
        ShaderUniformCache& shaderCache = uniform_cache_get_shader_cache(renderer.uniformCache, shader.get());
        shader_uniform_cache_insert_or_update(shaderCache, name, variable);
    }

    void textAlign(TextAlign textAlign)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.textAlign = textAlign;
    }

    void textFont(std::shared_ptr<Font> font)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.font = font;
    }

    void noTextFont()
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.font.reset();
    }

    void textSize(float size)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.textSize = size;
    }

    void tint(int grey, int alpha) { tint(rgba(grey, grey, grey, alpha)); }
    void tint(int red, int green, int blue, int alpha) { tint(rgba(red, green, blue, alpha)); }
    void tint(color_t color)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.tintColor = color;
    }

    void noTint()
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        renderState.tintColor = rgba(255, 255, 255);
    }
} // namespace p5

/// -----------------------------------------
///             Rendering methods
/// -----------------------------------------
namespace p5
{
    inline std::shared_ptr<Shader> get_current_shader(const RenderState& state)
    {
        auto result = (state.shader != nullptr) ? state.shader : defaultShader;
        assert(result != nullptr && "Current shader cannot be null");
        return result;
    }

    void background(int grey, int alpha) { background(rgba(grey, alpha)); }
    void background(int red, int green, int blue, int alpha) { background(rgba(red, green, blue, alpha)); }
    void background(color_t color)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        const uint2 size = renderer.framebuffer->getSize();
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
            DrawScope scope = draw_buffer_get_scope(renderer.drawBuffer);

            tesselate_quads(
                scope,
                PathPoints {
                    .size = 4,
                    .positions = positions,
                    .texcoords = texcoords,
                    .colors = colors,
                }
            );

            renderer_submit(
                renderer,
                scope,
                get_current_shader(renderState),
                renderState.blendMode,
                whiteTexture->getRendererId()
            );
        }
    }
} // namespace p5

namespace p5
{
    inline static std::shared_ptr<Font> get_current_font(const RenderState& state)
    {
        if (state.font != nullptr) {
            return state.font;
        }

        return defaultFont;
    }

    std::shared_ptr<Font> getCurrentFont()
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        return get_current_font(renderState);
    }
} // namespace p5

namespace p5
{
    static void recreateDefaultCanvas()
    {
        const int physW = window_physical_width(appWindow);
        const int physH = window_physical_height(appWindow);
        if (physW <= 0 || physH <= 0)
            return;

        defaultFramebuffer = create_window_framebuffer(physW, physH, appState.width, appState.height);
        needsDefaultCanvasRecreation = false;
        printf("Creating default canvas");
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
        if (renderer.drawBuffer.vertexCursor + SAFE_MARGIN_V >= renderer.drawBuffer.vertexCount ||
            renderer.drawBuffer.indexCursor + SAFE_MARGIN_I >= renderer.drawBuffer.indexCount) {
            renderer_flush(renderer);
        }
    }

    void pushCanvas(std::shared_ptr<Framebuffer> framebuffer)
    {
        // First we need to flush the renderer to make sure that all draw calls for the current canvas are submitted before we switch to the new canvas.
        renderer_flush(renderer);
        renderer_end_frame(renderer);

        framebufferStack.push_back(framebuffer);
        renderer_begin_frame(renderer, framebuffer);
        render_state_stack_push(renderStateStack, render_state_stack_peek(renderStateStack));
    }

    void popCanvas()
    {
        // Flush the current canvas before we pop it, to make sure that all draw calls for the current canvas are submitted before we switch back to the previous canvas.
        renderer_flush(renderer);
        renderer_end_frame(renderer);
        render_state_stack_pop(renderStateStack);

        framebufferStack.pop_back();
        if (not framebufferStack.empty()) {
            renderer_begin_frame(renderer, framebufferStack.back());
        }
    }

    void beginShape()
    {
    }

    // fillType  controls how the fill is tessellated into triangles.
    // strokeType controls how the outline is stroked — can differ from fillType
    // (e.g. fill a triangleFan but stroke only the outer lineLoop).
    void endShapeImpl(ShapeType fillType, ShapeType strokeType, std::optional<ColorStyle> fillStyle, std::optional<ColorStyle> strokeStyle, bool close)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);

        flushIfNeeded();
        DrawScope scope = draw_buffer_get_scope(renderer.drawBuffer);

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

        renderer_submit(renderer, scope, get_current_shader(renderState), renderState.blendMode, whiteTexture->getRendererId());

        linepath->clear();
        curveVertexCount = 0;
    }

    void endShape(ShapeType type, bool close)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
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
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        const float2 transformed = transformPoint(peekMatrix(), {x, y});
        linepath->vertex(transformed.x, transformed.y, u, v, renderState.fillColor, renderState.strokeColor);
    }

    void curveVertex(float x, float y)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
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

        RenderState& renderState = render_state_stack_peek(renderStateStack);

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
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        const size_t segmentCount = computeCircleSegmentCount(2.0f * std::numbers::pi_v<float>, std::max(width, height) * 0.5f);

        // Fill: centre vertex + closed rim → triangleFan (no libtess2, O(n) vertices)
        if (!renderState.isFillDisabled) {
            beginShape();
            vertex(centerX, centerY);
            for (size_t i = 0; i <= segmentCount; ++i) {
                float angle = 2.0f * std::numbers::pi_v<float> / static_cast<float>(segmentCount) * static_cast<float>(i);
                vertex(centerX + std::cos(angle) * width * 0.5f, centerY + std::sin(angle) * height * 0.5f);
            }
            endShapeImpl(ShapeType::triangleFan, ShapeType::triangleFan, ColorStyle::fill, std::nullopt, false);
        }

        // Stroke: rim only → lineLoop, so only the outer edge is stroked, not the internal fan edges
        if (!renderState.isStrokeDisabled) {
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
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        if (renderState.isStrokeDisabled) return;

        const size_t segmentCount = computeCircleSegmentCount(2.0f * std::numbers::pi_v<float>, renderState.strokeWeight * 0.5f);

        // Fill a disc with the stroke color as a triangleFan — no libtess2, no outline edges
        beginShape();
        vertex(x, y);
        for (size_t i = 0; i <= segmentCount; ++i) {
            float angle = 2.0f * std::numbers::pi_v<float> / static_cast<float>(segmentCount) * static_cast<float>(i);
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
        RenderState& renderState = render_state_stack_peek(renderStateStack);

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
        RenderState& renderState = render_state_stack_peek(renderStateStack);

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

    void image(uint32_t textureId, float left, float top, float width, float height)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        const std::array<float2, 4> positions = {
            transformPoint(peekMatrix(), {left, top}),
            transformPoint(peekMatrix(), {left + width, top}),
            transformPoint(peekMatrix(), {left + width, top + height}),
            transformPoint(peekMatrix(), {left, top + height}),
        };
        const std::array<float2, 4> texcoords = {
            float2 {0.0f, 0.0f},
            float2 {1.0f, 0.0f},
            float2 {1.0f, 1.0f},
            float2 {0.0f, 1.0f}
        };
        const std::array<color_t, 4> colors = {
            renderState.tintColor,
            renderState.tintColor,
            renderState.tintColor,
            renderState.tintColor
        };

        {
            flushIfNeeded();
            DrawScope scope = draw_buffer_get_scope(renderer.drawBuffer);

            tesselate_quads(
                scope,
                PathPoints {
                    .size = 4,
                    .positions = positions,
                    .texcoords = texcoords,
                    .colors = colors,
                }
            );

            renderer_submit(renderer, scope, get_current_shader(renderState), renderState.blendMode, textureId);
        }
    }

    void text(std::string_view text, float x, float y)
    {
        if (text.empty()) return;

        const std::u32string u32String = utf8ToUtf32(text);

        RenderState& renderState = render_state_stack_peek(renderStateStack);
        if (renderState.isFillDisabled) return; // fill color is the text color

        Font* font = get_current_font(renderState).get();

        TextMetrics metrics = measureText(text, font, renderState.textSize, 1.0f);

        // The horizontal start position for every line (alignment is against the full text width).
        float lineStartX;
        switch (renderState.textAlign.horizontal) {
            case HorizontalTextAlign::left: lineStartX = x; break;
            case HorizontalTextAlign::center: lineStartX = x - metrics.width * 0.5f; break;
            case HorizontalTextAlign::right: lineStartX = x - metrics.width; break;
        }

        float py;
        switch (renderState.textAlign.vertical) {
            case VerticalTextAlign::top: py = y + metrics.ascender; break;
            case VerticalTextAlign::center: py = y + metrics.ascender - metrics.totalHeight * 0.5f; break;
            case VerticalTextAlign::baseline: py = y; break;
            case VerticalTextAlign::bottom: py = y - metrics.totalHeight + metrics.ascender; break;
        }

        const Glyph* spaceGlyph = font->getGlyph(U' ', static_cast<int>(renderState.textSize));
        float px = lineStartX;
        bool isFirstCharOnLine = true; // tracks whether we need to apply bearing correction for this line

        char32_t prevCh = 0;
        for (size_t i = 0; i < u32String.length(); ++i) {
            const char32_t ch = u32String.at(i);

            if (ch == U'\n') {
                px = lineStartX; // reset to alignment-adjusted start, not raw x
                py += font->getLineHeight(renderState.textSize);
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

            const Glyph* glyph = font->getGlyph(ch, static_cast<int>(renderState.textSize));
            if (glyph == nullptr) {
                prevCh = ch;
                continue;
            }

            if (isFirstCharOnLine) {
                // Shift the pen left so the first glyph's visual left edge starts exactly at lineStartX.
                px -= glyph->bearing.x;
                isFirstCharOnLine = false;
            } else {
                px += font->getKerning(prevCh, ch, static_cast<int>(renderState.textSize));
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
                renderState.fillColor,
                renderState.fillColor,
                renderState.fillColor,
                renderState.fillColor
            };

            {
                flushIfNeeded();
                DrawScope scope = draw_buffer_get_scope(renderer.drawBuffer);
                tesselate_quads(
                    scope,
                    PathPoints {
                        .size = 4,
                        .positions = positions,
                        .texcoords = texcoords,
                        .colors = colors,
                    }
                );

                std::shared_ptr<Shader> shader = renderState.shader != nullptr ? renderState.shader : textShader;
                renderer_submit(renderer, scope, std::move(shader), renderState.blendMode, font->getGlyphPageTextureId(glyph->pageIndex));
            }

            px += glyph->advance.x;
            py += glyph->advance.y;
            prevCh = ch;
        }
    }
} // namespace p5

namespace p5
{
    inline static float s_targetFrameTime = 0.0f;
    inline static auto s_appStartTime = std::chrono::steady_clock::now();
    inline static bool s_isAppPaused = false;
    inline static bool s_isCloseRequested = false;
    inline static int s_exitCode = 0;

    void setWindowSize(int w, int h)
    {
        window_set_size(appWindow, w, h);
        appState.width = w;
        appState.height = h;
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

    Pixels loadPixels()
    {
        renderer_flush(renderer);

        const auto [viewportWidth, viewportHeight] = renderer.framebuffer->getViewportSize();
        const size_t count = static_cast<size_t>(viewportWidth) * static_cast<size_t>(viewportHeight);

        pixelScratch.resize(count);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, renderer.framebuffer->getRendererId());
        glReadPixels(0, 0, static_cast<GLsizei>(viewportWidth), static_cast<GLsizei>(viewportHeight), GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, pixelScratch.data());

        std::vector<color_t> buffer(count);
        for (uint32_t y = 0; y < viewportHeight; ++y) {
            const uint32_t flippedY = viewportHeight - 1 - y;
            std::copy_n(pixelScratch.data() + flippedY * viewportWidth, viewportWidth, buffer.data() + y * viewportWidth);
        }

        return Pixels {static_cast<int>(viewportWidth), static_cast<int>(viewportHeight), std::move(buffer)};
    }

    void updatePixels(const Pixels& pixels)
    {
        if (pixels.width <= 0 or pixels.height <= 0) {
            return;
        }

        const auto [viewportWidth, viewportHeight] = renderer.framebuffer->getViewportSize();
        if (static_cast<uint32_t>(pixels.width) != viewportWidth or static_cast<uint32_t>(pixels.height) != viewportHeight) {
            return;
        }

        const uint32_t canvasWidth = static_cast<uint32_t>(pixels.width);
        const uint32_t canvasHeight = static_cast<uint32_t>(pixels.height);
        const size_t count = static_cast<size_t>(canvasWidth) * static_cast<size_t>(canvasHeight);

        // Flip rows back to OpenGL bottom - up order
        pixelScratch.resize(count);
        for (uint32_t y = 0; y < canvasHeight; ++y) {
            const uint32_t flippedY = canvasHeight - 1 - y;
            std::copy_n(pixels.colors.data() + y * canvasWidth, canvasWidth, pixelScratch.data() + flippedY * canvasWidth);
        }

        renderer_flush(renderer);
        renderer.framebuffer->getColorTexture()->update(pixelScratch);
    }

} // namespace p5

namespace p5
{
    TextMetrics measureText(std::string_view text)
    {
        RenderState& renderState = render_state_stack_peek(renderStateStack);
        return measureText(text, get_current_font(renderState).get(), renderState.textSize, 1.0f);
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

        const std::u32string u32String = utf8ToUtf32(text);

        char32_t prevCodepoint = 0;
        for (size_t i = 0; i < u32String.length(); ++i) {
            const char32_t codepoint = u32String.at(i);

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

    std::vector<TextOutlinePoint> queryTextOutline(std::string_view text, int textSize, int curveSteps, float pointSpacing)
    {
        const std::u32string u32String = utf8ToUtf32(text);

        char32_t prevCodepoint = 0;
        for (size_t i = 0; i < u32String.length(); ++i) {
            const char32_t codepoint = u32String.at(i);
        }
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
            appState.mouseX = e.mouseMove.x;
            appState.mouseY = e.mouseMove.y;
        }
        if (e.type == EventType::windowResize) {
            appState.width = e.windowResize.width;
            appState.height = e.windowResize.height;
            needsDefaultCanvasRecreation = true;
        }
        if (e.type == EventType::framebufferResize) {
            needsDefaultCanvasRecreation = true;
        }
        sketch->event(e);
    });

    appState.width = window_logical_width(appWindow);
    appState.height = window_logical_height(appWindow);

    static constexpr size_t MAX_VERTICES = 65536;
    static constexpr size_t MAX_INDICES = MAX_VERTICES * 3;
    static constexpr uint8_t whitePixel[] = {255, 255, 255, 255};

    renderer = renderer_create(MAX_VERTICES, MAX_INDICES);
    whiteTexture = createTexture(1, 1, whitePixel);
    linepath = std::make_unique<LinePathBuilder>();
    defaultShader = create_default_shader();
    textShader = create_text_shader();
    defaultFont = loadFont({DejaVuSans_ttf, DejaVuSans_ttf_len});
    defaultFramebuffer = create_window_framebuffer(window_physical_width(appWindow), window_physical_height(appWindow), window_logical_width(appWindow), window_logical_height(appWindow));
    renderStateStack = render_state_stack_create();

    {
        pushCanvas(defaultFramebuffer);
        sketch->setup();
        popCanvas();
    }

    window_show(appWindow);

    FpsCounter fpsCounter;
    auto lastFrameTime = Clock::now();

    while (not s_isCloseRequested) {
        const TimePoint frameStart = Clock::now();
        appState.deltaTime = std::chrono::duration<float>(frameStart - lastFrameTime).count();
        lastFrameTime = frameStart;

        window_poll_events(appWindow);

        if (needsDefaultCanvasRecreation)
            recreateDefaultCanvas();

        if (not s_isAppPaused) {
            pushCanvas(defaultFramebuffer);
            sketch->draw();
            popCanvas();

            blit_framebuffer_to_screen(*defaultFramebuffer);
            window_swap_buffers(appWindow);
            appState.frameCount++;

            fpsCounter.tick();
            appState.frameRate = fpsCounter.fps();
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
