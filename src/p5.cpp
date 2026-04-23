#include "p5.hpp"
#include "renderer.hpp"
#include <numbers>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <iostream>
#include <stack>
#include <vector>

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
    inline static std::vector<DrawPoint> points;
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
        const DrawPoint points[] = {
            DrawPoint {.position = {0.0f, 0.0f}, .texcoord = {0.0f, 0.0f}, .fillColor = color, .strokeColor = p5::color(0, 0, 0, 0)},
            DrawPoint {.position = {800.0f, 0.0f}, .texcoord = {0.0f, 0.0f}, .fillColor = color, .strokeColor = p5::color(0, 0, 0, 0)},
            DrawPoint {.position = {800.0f, 600.0f}, .texcoord = {0.0f, 0.0f}, .fillColor = color, .strokeColor = p5::color(0, 0, 0, 0)},
            DrawPoint {.position = {0.0f, 600.0f}, .texcoord = {0.0f, 0.0f}, .fillColor = color, .strokeColor = p5::color(0, 0, 0, 0)},
        };

        const RenderState& state = peekState();
        const DrawSettings settings = {
            .shaderId = std::nullopt,
            .textureId = std::nullopt,
            .blendMode = state.blendMode,
            .drawMode = DrawMode::triangles,
        };

        renderer->draw(settings, [&points](DrawScope& scope) {
            convex(scope, points);
            // concave(scope, points);
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

    void endShape()
    {
        const RenderState& state = peekState();

        DrawSettings settings = {
            .shaderId = std::nullopt,
            .textureId = std::nullopt,
            .blendMode = state.blendMode,
            .drawMode = DrawMode::triangles,
        };

        renderer->draw(settings, [](DrawScope& scope) {
            // convex(scope, points);
            concave(scope, points);
        });

        points.clear();
    }

    void vertex(float x, float y)
    {
        vertex(x, y, 0.0f, 0.0f);
    }

    void vertex(float x, float y, float u, float v)
    {
        const RenderState& state = peekState();

        points.emplace_back(DrawPoint {
            .position = float2 {.x = x, .y = y},
            .texcoord = float2 {.x = u, .y = v},
            .fillColor = state.fillColor,
            .strokeColor = state.strokeColor,
        });
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
        // Radien begrenzen damit sie sich nicht überlappen
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

    void triangle(float x1, float y1, float x2, float y2, float x3, float y3)
    {
        beginShape();
        vertex(x1, y1);
        vertex(x2, y2);
        vertex(x3, y3);
        endShape();
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
