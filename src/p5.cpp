#include "p5.hpp"
#include "geometry.hpp"
#include "p5_internal.hpp"
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

    inline DrawSubmission buildDrawSubmission(const RenderState& state)
    {
        return DrawSubmission {
            .shaderId = std::nullopt,
            .textureId = std::nullopt,
            .blendMode = state.blendMode,
        };
    }

    struct Window
    {
        GLFWwindow* handle;
        int framebufferWidth;
        int framebufferHeight;
        int windowWidth;
        int windowHeight;
    };

    inline static std::stack<RenderState> renderStates;
    inline static std::unique_ptr<Renderer> renderer;
    inline static std::vector<GeometryPoint> shapeVertices;
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
        renderer->draw({}, [color](GeometryBuilder& builder) {
            const GeometryPoint vertices[] = {
                GeometryPoint {.position = {0.0f, 0.0f}, .texcoord = {0.0f, 0.0f}, .fillColor = color, .strokeColor = p5::color(0, 0, 0, 0)},
                GeometryPoint {.position = {0.0f, 600.0f}, .texcoord = {0.0f, 0.0f}, .fillColor = color, .strokeColor = p5::color(0, 0, 0, 0)},
                GeometryPoint {.position = {800.0f, 600.0f}, .texcoord = {0.0f, 0.0f}, .fillColor = color, .strokeColor = p5::color(0, 0, 0, 0)},
                GeometryPoint {.position = {800.0f, 0.0f}, .texcoord = {0.0f, 0.0f}, .fillColor = color, .strokeColor = p5::color(0, 0, 0, 0)},
            };
            // const GeometryPoint vertices[] = {
            //     GeometryPoint {.position = {0.0f, 0.0f}, .texcoord = {0.0f, 0.0f}, .fillColor = color, .strokeColor = p5::color(0, 0, 0, 0)},
            //     GeometryPoint {.position = {800.0f, 0.0f}, .texcoord = {0.0f, 0.0f}, .fillColor = color, .strokeColor = p5::color(0, 0, 0, 0)},
            //     GeometryPoint {.position = {800.0f, 600.0f}, .texcoord = {0.0f, 0.0f}, .fillColor = color, .strokeColor = p5::color(0, 0, 0, 0)},
            //     GeometryPoint {.position = {0.0f, 600.0f}, .texcoord = {0.0f, 0.0f}, .fillColor = color, .strokeColor = p5::color(0, 0, 0, 0)},
            // };

            builder.concave(vertices);
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

        if (state.isFillEnabled) {
            renderer->draw(
                buildDrawSubmission(state),
                [](GeometryBuilder& builder) {
                    builder.convex(shapeVertices);
                }
            );
        }

        shapeVertices.clear();
    }

    void vertex(float x, float y)
    {
        vertex(x, y, 0.0f, 0.0f);
    }

    void vertex(float x, float y, float u, float v)
    {
        const RenderState& state = peekState();

        shapeVertices.emplace_back(GeometryPoint {
            .position = {x, y},
            .texcoord = {u, v},
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

    GeometryBuffer buffer {
        .vertices = std::make_unique<Vertex[]>(10'000),
        .vertexCount = 10'000,
        .indices = std::make_unique<uint32_t[]>(30'000),
        .indexCount = 30'000,
    };

    renderStates.push(RenderState {});
    renderer = Renderer::create(buffer, 100);

    const std::unique_ptr sketch = createSketch();
    sketch->setup();

    glfwShowWindow(window.handle);

    while (not glfwWindowShouldClose(window.handle)) {
        glfwPollEvents();

        renderer->beginDraw();
        sketch->draw();
        renderer->endDraw();

        glfwSwapBuffers(window.handle);
    }

    sketch->destroy();
    renderer.reset();

    glfwTerminate();

    return 0;
}
