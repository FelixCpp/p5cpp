#include <p5cpp.hpp>

#include "app_context.hpp"
#include "engine.hpp"
#include "modules/frame/frame_data.hpp"
#include "modules/frame/frame_module.hpp"
#include "modules/input/input_data.hpp"
#include "modules/input/input_module.hpp"
#include "modules/rendering/rendering_data.hpp"
#include "modules/rendering/rendering_module.hpp"
#include "modules/sketch/sketch_module.hpp"
#include "modules/window/window_module.hpp"
#include "render_state_stack.hpp"
#include "services/renderer.hpp"
#include "services/window.hpp"
#include "utf8_view.hpp"
#include "linepath.hpp"
#include "render_state.hpp"
#include "tess.hpp"

#include <cassert>
#include <iostream>
#include <array>
#include <numbers>
#include <algorithm>

namespace p5cpp
{
    static std::unique_ptr<Engine> engine;

    inline AppContext& getAppContext()
    {
        return engine->getContext();
    }
} // namespace p5cpp

namespace p5cpp
{
    void info(std::string_view message) { std::cout << "[INFO]: " << message << std::endl; }
    void debug(std::string_view message) { std::cout << "[DEBUG]: " << message << std::endl; }
    void warning(std::string_view message) { std::cout << "[WARNING]: " << message << std::endl; }
    void error(std::string_view message) { std::cerr << "[ERROR]: " << message << std::endl; }
} // namespace p5cpp

namespace p5cpp
{
    void setWindowSize(int width, int height) { getAppContext().require<Window>().setSize(width, height); }
    void setWindowTitle(std::string_view title) { getAppContext().require<Window>().setTitle(title); }
    void setWindowResizable(bool resizable) { getAppContext().require<Window>().setResizable(resizable); }

    int getMouseX() { return getAppContext().require<InputData>().mouseX; }
    int getMouseY() { return getAppContext().require<InputData>().mouseY; }
    int getPMouseX() { return getAppContext().require<InputData>().pmouseX; }
    int getPMouseY() { return getAppContext().require<InputData>().pmouseY; }

    int getWidth() { return getAppContext().require<InputData>().logicalWidth; }
    int getHeight() { return getAppContext().require<InputData>().logicalHeight; }
    int getWindowWidth() { return getAppContext().require<InputData>().physicalWidth; }
    int getWindowHeight() { return getAppContext().require<InputData>().physicalHeight; }
} // namespace p5cpp

namespace p5cpp
{
    void frameRate(int targetFps) { getAppContext().require<FrameData>().targetFrameRate = targetFps; }
    void loop() { getAppContext().require<FrameData>().isPaused = false; }
    void noLoop() { getAppContext().require<FrameData>().isPaused = true; }
    bool isLooping() { return not getAppContext().require<FrameData>().isPaused; }
    void quit() { getAppContext().require<FrameData>().closeRequested = true; }
    void quit(int code) { exitCode(code), quit(); }
    void exitCode(int code) { getAppContext().require<FrameData>().exitCode = code; }
    int getFrameCount() { return getAppContext().require<FrameData>().frameCount; }
    int getFrameRate() { return getAppContext().require<FrameData>().framesPerSecond; }
    float getDeltaTime() { return getAppContext().require<FrameData>().deltaTime; }
    float getGlobalTime() { return getAppContext().require<FrameData>().globalTime; }
} // namespace p5cpp

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
    inline Renderer& getRenderer()
    {
        return *engine->getContext().require<RenderingData>().renderer;
    }

    inline RenderStateStack& getRenderStateStack()
    {
        return engine->getContext().require<RenderingData>().renderStateStack;
    } // namespace p5cpp
} // namespace p5cpp

namespace p5cpp
{
} // namespace p5cpp

namespace p5cpp
{
    inline static size_t curveVertexCount;
    inline static std::array<float2, 4> curveVertexPositions;

    inline static std::unique_ptr<LinePathBuilder> linepath;
    inline static std::vector<std::shared_ptr<Framebuffer>> framebufferStack;
} // namespace p5cpp

/// --------------------------------------------
///               RenderState Options
/// --------------------------------------------
namespace p5cpp
{
    void pushState()
    {
        render_state_stack_push(getRenderStateStack(), render_state_stack_peek(getRenderStateStack()));
    }

    void popState()
    {
        render_state_stack_pop(getRenderStateStack());
    }

    void pushMatrix()
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        matrix_stack_push(renderState.metrics, matrix_stack_peek(renderState.metrics));
    }

    void popMatrix()
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        matrix_stack_pop(renderState.metrics);
    }

    void resetMatrix()
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        matrix_stack_set(renderState.metrics, matrix4x4::identity);
    }

    void applyMatrix(const matrix4x4& matrix)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        matrix_stack_apply(renderState.metrics, matrix);
    }

    void setMatrix(const matrix4x4& matrix)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        matrix_stack_set(renderState.metrics, matrix);
    }

    matrix4x4& peekMatrix()
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        return matrix_stack_peek(renderState.metrics);
    }

    void translate(float x, float y)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        matrix_stack_apply(renderState.metrics, translation(x, y));
    }

    void scale(float x, float y)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        matrix_stack_apply(renderState.metrics, scaling(x, y));
    }

    void rotate(float angle)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        matrix_stack_apply(renderState.metrics, rotation(angle));
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
        getAppContext().require<RenderingData>().uniformCache->setUniform(shader.get(), name, variable);
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

/// -----------------------------------------
///             Rendering methods
/// -----------------------------------------
namespace p5cpp
{
    inline Shader* get_current_shader(const RenderState& state)
    {
        auto result = (state.shader != nullptr) ? state.shader.get() : getAppContext().require<RenderingData>().defaultShader.get();
        assert(result != nullptr && "Current shader cannot be null");
        return result;
    }

    inline Shader* get_current_text_shader(const RenderState& state)
    {
        auto result = (state.shader != nullptr) ? state.shader.get() : getAppContext().require<RenderingData>().textShader.get();
        assert(result != nullptr && "Current text shader cannot be null");
        return result;
    }

    void background(int grey, int alpha) { background(rgba(grey, alpha)); }
    void background(int red, int green, int blue, int alpha) { background(rgba(red, green, blue, alpha)); }
    void background(color_t color)
    {
        Renderer& renderer = getRenderer();
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        const uint2 size = framebufferStack.back()->getSize();
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
                *getAppContext().require<RenderingData>().uniformCache,
                get_current_shader(renderState),
                renderState.blendMode,
                getAppContext().require<RenderingData>().whiteTexture.get()
            );
        }
    }
} // namespace p5cpp

namespace p5cpp
{
    inline static Font* get_current_font(const RenderState& state)
    {
        if (state.font != nullptr) {
            return state.font.get();
        }

        return getAppContext().require<RenderingData>().defaultFont.get();
    }

    Font* getCurrentFont()
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        return get_current_font(renderState);
    }
} // namespace p5cpp

namespace p5cpp
{
    UniformVariable uniform(float x) { return UniformVariable {.type = UniformVariable::Type::float1, .floatValue = x}; }
    UniformVariable uniform(float x, float y) { return UniformVariable {.type = UniformVariable::Type::float2, .float2Value = float2 {x, y}}; }
    UniformVariable uniform(float x, float y, float z, float w) { return UniformVariable {.type = UniformVariable::Type::float4, .float4Value = float4 {x, y, z, w}}; }
    UniformVariable uniform(const matrix4x4& value) { return UniformVariable {.type = UniformVariable::Type::matrix4x4, .matrix4x4Value = value}; }

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
            getRenderer().flush();
        }
    }

    void pushCanvas(std::shared_ptr<Framebuffer> framebuffer)
    {
        // First we need to flush the getRenderer() to make sure that all draw calls for the current canvas are submitted before we switch to the new canvas.
        Renderer& renderer = getRenderer();
        renderer.flush();
        renderer.end();

        framebufferStack.push_back(framebuffer);
        renderer.begin(framebuffer.get());
        render_state_stack_push(getRenderStateStack(), render_state_stack_peek(getRenderStateStack()));
    }

    void popCanvas()
    {
        // Flush the current canvas before we pop it, to make sure that all draw calls for the current canvas are submitted before we switch back to the previous canvas.
        Renderer& renderer = getRenderer();
        renderer.flush();
        renderer.end();

        render_state_stack_pop(getRenderStateStack());

        framebufferStack.pop_back();
        if (not framebufferStack.empty()) {
            renderer.begin(framebufferStack.back().get());
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
        Renderer& renderer = getRenderer();
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        DrawScope scope = renderer.getDrawScope();
        flushIfNeeded(scope);

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

        renderer.submit(scope, *getAppContext().require<RenderingData>().uniformCache, get_current_shader(renderState), renderState.blendMode, getAppContext().require<RenderingData>().whiteTexture.get());

        linepath->clear();
        curveVertexCount = 0;
    }

    void endShape(ShapeType type, bool close)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
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
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        const float2 transformed = transformPoint(peekMatrix(), {x, y});
        linepath->vertex(transformed.x, transformed.y, u, v, renderState.fillColor, renderState.strokeColor);
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
        const size_t segmentCount = computeCircleSegmentCount(2.0f * std::numbers::pi_v<float>, std::max(width, height) * 0.5f);

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
        const size_t segmentCount = computeCircleSegmentCount(2.0f * std::numbers::pi_v<float>, renderState.strokeWeight * 0.5f);

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

    void image(Texture* texture, float left, float top, float width, float height)
    {
        Renderer& renderer = getRenderer();
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        const std::array<float2, 4> positions = {
            transformPoint(matrix_stack_peek(renderState.metrics), {left, top}),
            transformPoint(matrix_stack_peek(renderState.metrics), {left + width, top}),
            transformPoint(matrix_stack_peek(renderState.metrics), {left + width, top + height}),
            transformPoint(matrix_stack_peek(renderState.metrics), {left, top + height}),
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

            renderer.submit(scope, *getAppContext().require<RenderingData>().uniformCache, get_current_shader(renderState), renderState.blendMode, texture);
        }
    }

    void text(std::string_view text, float x, float y, std::optional<float> maxWidth)
    {
        Renderer& renderer = getRenderer();
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        Font* font = get_current_font(renderState);
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

            bucket.positions.push_back(transformPoint(matrix, {x + quad.vertexRect.left, y + quad.vertexRect.top}));
            bucket.positions.push_back(transformPoint(matrix, {x + quad.vertexRect.left + quad.vertexRect.width, y + quad.vertexRect.top}));
            bucket.positions.push_back(transformPoint(matrix, {x + quad.vertexRect.left + quad.vertexRect.width, y + quad.vertexRect.top + quad.vertexRect.height}));
            bucket.positions.push_back(transformPoint(matrix, {x + quad.vertexRect.left, y + quad.vertexRect.top + quad.vertexRect.height}));

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

            renderer.submit(scope, *getAppContext().require<RenderingData>().uniformCache, get_current_text_shader(renderState), renderState.blendMode, texture);
        }
    }
} // namespace p5cpp

namespace p5cpp
{
    TextLayout measureText(std::string_view text)
    {
        RenderState& renderState = render_state_stack_peek(getRenderStateStack());
        return measureText(
            text,
            get_current_font(renderState),
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
} // namespace p5cpp

int main()
{
    using namespace p5cpp;

    linepath = std::make_unique<LinePathBuilder>();
    framebufferStack = {};

    engine = Engine::create();
    engine->addModule(std::make_unique<FrameModule>());
    engine->addModule(std::make_unique<WindowModule>());
    engine->addModule(std::make_unique<InputModule>());
    engine->addModule(std::make_unique<RenderingModule>());
    engine->addModule(std::make_unique<SketchModule>());

    engine->run();

    return 0;
}
