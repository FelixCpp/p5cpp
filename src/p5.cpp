#include "p5.hpp"
#include "renderer.hpp"
#include <numbers>
#include <algorithm>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <iostream>
#include <stack>
#include <vector>
#include <array>

namespace p5
{
    color_t color(int grey, int alpha) { return color(grey, grey, grey, alpha); }
    color_t color(int red, int green, int blue, int alpha) { return (red << 24) | (green << 16) | (blue << 8) | alpha; } // TODO(Felix): Clamp values to 0 .. 255

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
        StrokeAlign strokeAlign = StrokeAlign::center;
        float miterLimit = 10.0f;

        BlendMode blendMode = BlendMode::alpha;
        AngleMode angleMode = AngleMode::degrees;

        bool isFillEnabled = true;
        bool isStrokeEnabled = true;
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
    inline Window window;

    inline RenderState& peekState() { return renderStates.top(); }

    void pushState() { renderStates.push(peekState()); }
    void popState()
    {
        if (renderStates.size() > 1) renderStates.pop();
    }

    void background(int grey, int alpha) { background(grey, grey, grey, alpha); }
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
            std::array<float2, 4> positions = {
                float2 {0.0f, 0.0f},
                float2 {800.0f, 0.0f},
                float2 {800.0f, 600.0f},
                float2 {0.0f, 600.0f}
            };

            std::array<float2, 4> texcoords = {
                float2 {0.0f, 0.0f},
                float2 {0.0f, 0.0f},
                float2 {0.0f, 0.0f},
                float2 {0.0f, 0.0f}
            };

            std::array<color_t, 4> colors = {color, color, color, color};

            concave(
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

    void noFill() { peekState().isFillEnabled = false; }
    void fill(int grey, int alpha) { fill(grey, grey, grey, alpha); }
    void fill(int red, int green, int blue, int alpha) { fill(color(red, green, blue, alpha)); }
    void fill(color_t color)
    {
        RenderState& state = peekState();
        state.fillColor = color;
        state.isFillEnabled = true;
    }

    void noStroke() { peekState().isStrokeEnabled = false; }
    void stroke(int grey, int alpha) { stroke(grey, grey, grey, alpha); }
    void stroke(int red, int green, int blue, int alpha) { stroke(color(red, green, blue, alpha)); }
    void stroke(color_t color)
    {
        RenderState& state = peekState();
        state.strokeColor = color;
        state.isStrokeEnabled = true;
    }

    void strokeWeight(float strokeWeight) { peekState().strokeWeight = strokeWeight; }
    void strokeCap(StrokeCap strokeCap) { peekState().strokeCap = strokeCap; }
    void strokeJoin(StrokeJoin strokeJoin) { peekState().strokeJoin = strokeJoin; }
    void strokeAlign(StrokeAlign strokeAlign) { peekState().strokeAlign = strokeAlign; }
    void miterLimit(float miterLimit) { peekState().miterLimit = miterLimit; }
    void blendMode(BlendMode blendMode) { peekState().blendMode = blendMode; }

    void beginShape()
    {
    }

    enum class FillStyle {
        fill,
        stroke
    };
    void endShapeFillOnly(FillStyle style);

    void endShape()
    {
        // TODO(Felix): handle endShape gracefully
        endShapeFillOnly(FillStyle::fill);
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
            concave(
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
        drawPointPositions[drawPointCount] = {x, y};
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
        beginShape();
        for (size_t i = 0; i < 32; ++i) {
            float angle = 2.0f * std::numbers::pi_v<float> / 32.0f * i;
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

        beginShape();
        for (size_t i = 0; i < 32; ++i) {
            float angle = 2.0f * std::numbers::pi_v<float> / 32.0f * i;
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

        const float dx = x2 - x1;
        const float dy = y2 - y1;
        const float len = std::sqrt(dx * dx + dy * dy);

        if (len == 0.0f) return;

        const float ndx = dx / len;
        const float ndy = dy / len;

        if (state.strokeCap == StrokeCap::square) {
            x1 -= ndx * state.strokeWeight * 0.5f;
            y1 -= ndy * state.strokeWeight * 0.5f;
            x2 += ndx * state.strokeWeight * 0.5f;
            y2 += ndy * state.strokeWeight * 0.5f;
        }

        const float px = -ndy;
        const float py = ndx;

        const float ox = px * state.strokeWeight * 0.5f;
        const float oy = py * state.strokeWeight * 0.5f;

        beginShape();
        if (state.strokeCap != StrokeCap::round) {
            vertex(x1 + ox, y1 + oy);
            vertex(x2 + ox, y2 + oy);
            vertex(x2 - ox, y2 - oy);
            vertex(x1 - ox, y1 - oy);

        } else {
            const size_t segments = 8;
            float baseAngle = std::atan2(dy, dx);
            float radius = state.strokeWeight * 0.5f;

            for (size_t i = 0; i <= segments; ++i) {
                float t = static_cast<float>(i) / segments;
                float a = baseAngle + std::numbers::pi_v<float> * 0.5f + t * std::numbers::pi_v<float>;

                float x = x1 + std::cos(a) * radius;
                float y = y1 + std::sin(a) * radius;

                vertex(x, y);
            }

            for (size_t i = 0; i <= segments; ++i) {
                float t = static_cast<float>(i) / segments;
                float a = baseAngle - std::numbers::pi_v<float> * 0.5f + t * std::numbers::pi_v<float>;

                float x = x2 + std::cos(a) * radius;
                float y = y2 + std::sin(a) * radius;

                vertex(x, y);
            }
        }
        endShapeFillOnly(FillStyle::stroke);
    }
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

    window.handle = glfwCreateWindow(window.windowWidth, window.windowHeight, "p5", nullptr, nullptr);
    glfwMakeContextCurrent(window.handle);
    glfwSwapInterval(1);

    glfwSetWindowSizeCallback(window.handle, [](GLFWwindow* handle, int width, int height) {
        window.windowWidth = width;
        window.windowHeight = height;
    });

    glfwSetFramebufferSizeCallback(window.handle, [](GLFWwindow* handle, int width, int height) {
        window.framebufferWidth = width;
        window.framebufferHeight = height;
    });

    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&glfwGetProcAddress));

    glEnable(GL_MULTISAMPLE);

    std::cout << glGetString(GL_VERSION) << std::endl;
    std::cout << glGetString(GL_VENDOR) << std::endl;
    std::cout << glGetString(GL_RENDERER) << std::endl;
    std::cout << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    renderStates.push(RenderState {});
    renderer = createRenderer();

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
        renderer->beginDraw();
        sketch->draw();
        renderer->endDraw();
        glfwSwapBuffers(window.handle);
    }

    sketch->destroy();
    sketch.reset();
    glfwTerminate();

    return 0;
}
