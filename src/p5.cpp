#include "p5.hpp"
#include "renderer.hpp"
#include "tess.hpp"
#include "stroker.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <iostream>
#include <stack>
#include <vector>
#include <array>
#include <stack>
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

    size_t computeCircleSegmentCount(float angle, float radius)
    {
        const float arcLength = std::abs(angle) * radius;
        const size_t segmentCount = static_cast<size_t>(std::ceil(arcLength / 4.0f)); // 4.0f is the desired maximum segment length
        return std::max(segmentCount, static_cast<size_t>(4));                        // Minimum of 4 segments for a full circle
    }

    int red(color_t color) { return (color & 0xFF000000) >> 24; }
    int green(color_t color) { return (color & 0x00FF0000) >> 16; }
    int blue(color_t color) { return (color & 0x0000FF00) >> 8; }
    int alpha(color_t color) { return (color & 0x000000FF) >> 0; }
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
    struct RenderState
    {
        color_t fillColor = color(255, 255, 255);
        color_t strokeColor = color(255, 255, 255);

        float strokeWeight = 1.0f;
        StrokeCap strokeCap = StrokeCap::round;
        StrokeJoin strokeJoin = StrokeJoin::round;
        float miterLimit = 10.0f;

        BlendMode blendMode = BlendMode::alpha;

        std::stack<matrix4x4> metrics = std::stack<matrix4x4>({matrix4x4::identity});

        bool isFillDisabled = false;
        bool isStrokeDisabled = false;
    };

    struct Window
    {
        GLFWwindow* handle;
        int framebufferWidth;
        int framebufferHeight;
        int windowWidth;
        int windowHeight;
    };

    inline static std::stack<RenderState> renderStates;
    inline static size_t drawPointCount;
    inline static std::vector<float2> drawPointPositions;
    inline static std::vector<float2> drawPointTexCoords;
    inline static std::vector<color_t> drawPointFillColors;
    inline static std::vector<color_t> drawPointStrokeColors;
    inline static std::unique_ptr<Renderer> renderer;
    inline static std::unique_ptr<Tesselator> tesselator;
    inline static std::unique_ptr<Stroker> stroker;
    inline Window window;

    inline RenderState& peekState() { return renderStates.top(); }

    void pushState() { renderStates.push(peekState()); }
    void popState()
    {
        if (renderStates.size() > 1) renderStates.pop();
    }

    void pushMatrix()
    {
        RenderState& state = peekState();
        state.metrics.push(state.metrics.top());
    }

    void popMatrix()
    {
        RenderState& state = peekState();
        if (state.metrics.size() > 1) {
            state.metrics.pop();
        }
    }

    void resetMatrix()
    {
        RenderState& state = peekState();
        state.metrics.top() = matrix4x4::identity;
    }

    void applyMatrix(const matrix4x4& matrix)
    {
        RenderState& state = peekState();
        state.metrics.top() = combine(state.metrics.top(), matrix);
    }

    void setMatrix(const matrix4x4& matrix)
    {
        RenderState& state = peekState();
        state.metrics.top() = matrix;
    }

    matrix4x4& peekMatrix() { return peekState().metrics.top(); }

    void translate(float x, float y) { applyMatrix(translation(x, y)); }
    void scale(float x, float y) { applyMatrix(scaling(x, y)); }
    void rotate(float angle) { applyMatrix(rotation(angle)); }

    void background(int grey, int alpha) { background(color(grey, grey, grey, alpha)); }
    void background(int red, int green, int blue, int alpha) { background(color(red, green, blue, alpha)); }
    void background(color_t color)
    {
        const RenderState& state = peekState();
        const DrawSettings settings = {
            .shaderId = std::nullopt,
            .textureId = std::nullopt,
            .blendMode = state.blendMode,
            .drawMode = DrawMode::triangles,
        };

        renderer->draw(settings, [color](DrawScope& scope) {
            const std::array<float2, 4> positions = {
                float2 {0.0f, 0.0f},
                float2 {800.0f, 0.0f},
                float2 {800.0f, 600.0f},
                float2 {0.0f, 600.0f}
            };

            const std::array<float2, 4> texcoords = {
                float2 {0.0f, 0.0f},
                float2 {0.0f, 0.0f},
                float2 {0.0f, 0.0f},
                float2 {0.0f, 0.0f}
            };

            const std::array<color_t, 4> colors = {color, color, color, color};

            tesselator->tesselate(
                scope,
                DrawPoints {
                    .size = 4,
                    .positions = positions,
                    .texcoords = texcoords,
                    .colors = colors,
                }
            );
        });
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
    void miterLimit(float miterLimit) { peekState().miterLimit = miterLimit; }
    void blendMode(BlendMode blendMode) { peekState().blendMode = blendMode; }

    void beginShape()
    {
    }

    enum class FillStyle {
        fill,
        stroke
    };

    void endShape(bool close)
    {
        const RenderState& state = peekState();

        DrawSettings settings = {
            .shaderId = std::nullopt,
            .textureId = std::nullopt,
            .blendMode = state.blendMode,
            .drawMode = DrawMode::triangles,
        };

        renderer->draw(settings, [close, &state](DrawScope& scope) {
            if (not state.isFillDisabled) {
                tesselator->tesselate(
                    scope,
                    DrawPoints {
                        .size = drawPointCount,
                        .positions = drawPointPositions,
                        .texcoords = drawPointTexCoords,
                        .colors = drawPointFillColors
                    }
                );
            }

            if (not state.isStrokeDisabled) {
                stroker->stroke(
                    scope,
                    DrawPoints {
                        .size = drawPointCount,
                        .positions = drawPointPositions,
                        .texcoords = drawPointTexCoords,
                        .colors = drawPointStrokeColors,
                    },
                    state.strokeWeight,
                    state.strokeCap,
                    state.strokeJoin,
                    state.miterLimit,
                    close
                );
            }
        });

        drawPointCount = 0;
    }

    void endShapeFillOnly(FillStyle style)
    {
        const RenderState& state = peekState();

        DrawSettings settings = {
            .shaderId = std::nullopt,
            .textureId = std::nullopt,
            .blendMode = state.blendMode,
            .drawMode = DrawMode::triangles,
        };

        renderer->draw(settings, [style](DrawScope& scope) {
            tesselator->tesselate(
                scope,
                DrawPoints {
                    .size = drawPointCount,
                    .positions = drawPointPositions,
                    .texcoords = drawPointTexCoords,
                    .colors = style == FillStyle::fill ? drawPointFillColors : drawPointStrokeColors,
                }
            );
        });

        drawPointCount = 0;
    }

    void vertex(float x, float y)
    {
        vertex(x, y, 0.0f, 0.0f);
    }

    void vertex(float x, float y, float u, float v)
    {
        if (drawPointCount >= drawPointPositions.size()) drawPointPositions.resize(std::max(drawPointPositions.size() * 2, 1uz));
        if (drawPointCount >= drawPointTexCoords.size()) drawPointTexCoords.resize(std::max(drawPointTexCoords.size() * 2, 1uz));
        if (drawPointCount >= drawPointFillColors.size()) drawPointFillColors.resize(std::max(drawPointFillColors.size() * 2, 1uz));
        if (drawPointCount >= drawPointStrokeColors.size()) drawPointStrokeColors.resize(std::max(drawPointStrokeColors.size() * 2, 1uz));

        const RenderState& state = peekState();
        drawPointPositions[drawPointCount] = transformPoint(state.metrics.top(), {x, y});
        drawPointTexCoords[drawPointCount] = {u, v};
        drawPointFillColors[drawPointCount] = state.fillColor;
        drawPointStrokeColors[drawPointCount] = state.strokeColor;
        ++drawPointCount;
    }

    void rect(float left, float top, float width, float height)
    {
        beginShape();
        vertex(left, top);
        vertex(left + width, top);
        vertex(left + width, top + height);
        vertex(left, top + height);
        endShape();
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

        endShape();
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
        endShape();
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
        endShapeFillOnly(FillStyle::stroke);
    }

    void triangle(float x1, float y1, float x2, float y2, float x3, float y3)
    {
        beginShape();
        vertex(x1, y1);
        vertex(x2, y2);
        vertex(x3, y3);
        endShape();
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
        endShapeFillOnly(FillStyle::stroke);
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

    window.windowWidth = 800;
    window.windowHeight = 600;
    projectionMatrix = ortho(0.0f, static_cast<float>(window.windowWidth), static_cast<float>(window.windowHeight), 0.0f, -1.0f, 1.0f);

    window.handle = glfwCreateWindow(window.windowWidth, window.windowHeight, "p5", nullptr, nullptr);
    glfwMakeContextCurrent(window.handle);
    glfwSwapInterval(1);

    glfwSetWindowSizeCallback(window.handle, [](GLFWwindow* handle, int width, int height) {
        window.windowWidth = width;
        window.windowHeight = height;
        projectionMatrix = ortho(0.0f, static_cast<float>(window.windowWidth), static_cast<float>(window.windowHeight), 0.0f, -1.0f, 1.0f);
    });

    glfwSetFramebufferSizeCallback(window.handle, [](GLFWwindow* handle, int width, int height) {
        window.framebufferWidth = width;
        window.framebufferHeight = height;
        glViewport(0, 0, width, height);
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

    renderStates.push(RenderState {});
    renderer = createRenderer();
    tesselator = createTesselator();
    stroker = createStroker();

    static std::unique_ptr sketch = createSketch();
    sketch->setup();

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
        renderer->beginDraw(Camera {.projection = projectionMatrix});
        sketch->draw();
        renderer->endDraw();
        glfwSwapBuffers(window.handle);
    }

    sketch->destroy();
    sketch.reset();
    glfwTerminate();

    return 0;
}
