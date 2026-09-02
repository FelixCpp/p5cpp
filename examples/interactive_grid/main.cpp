#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

using namespace p5;
using namespace p5::animation;

struct InteractiveGrid : Sketch
{
    int cols;
    int rows;
    float spacing;
    float affectionRadius;

    void recomputeValues(uint32_t width, uint32_t height)
    {
        cols = static_cast<int>(std::floor(width / spacing));
        rows = static_cast<int>(std::floor(height / spacing));
    }

    void setup() override
    {
        setWindowSize(400, 400);

        spacing = 20.0f;
        recomputeValues(getWindowSize().x, getWindowSize().y);
        affectionRadius = static_cast<float>(std::min(getWindowSize().x, getWindowSize().y)) * 0.5f;
    }

    void event(const WindowEvent& event) override
    {
        if (event.is<WindowEvent::WindowResize>()) {
            auto& resize = event.as<WindowEvent::WindowResize>();
            recomputeValues(resize.width, resize.height);
        }

        if (event.is<WindowEvent::MouseScroll>()) {
            auto& scroll = event.as<WindowEvent::MouseScroll>();
            affectionRadius = std::max(0.0f, affectionRadius + static_cast<float>(scroll.yOffset) * 10.0f);
        }
    }

    void draw() override
    {
        background(rgba(0));

        std::vector<float> sizes((cols + 1) * (rows + 1), spacing);
        const float2 mousePosition = float2 {.x = static_cast<float>(getMouseX()), .y = static_cast<float>(getMouseY())};
        for (size_t y = 0; y <= rows; ++y) {
            for (size_t x = 0; x <= cols; ++x) {
                const float positionX = spacing * 0.5f + x * spacing;
                const float positionY = spacing * 0.5f + y * spacing;
                const float2 position = float2 {.x = positionX, .y = positionY};
                const float dist = distance(position, mousePosition);
                const float influence = curves::easeInSine(constrain(dist / affectionRadius, 0.0f, 1.0f));
                const float sizeFactor = 1.0f - influence;

                sizes[y * (cols + 1) + x] = sizeFactor * spacing;
            }
        }

        const float gridOffset = spacing * 0.5f;

        fill(rgba(220));
        noStroke();
        for (size_t y = 0; y <= rows; y++) {
            for (size_t x = 0; x <= cols; x++) {
                const float cellSize = sizes[y * (cols + 1) + x];
                const float left = x * spacing - cellSize / 2.0f;
                const float top = y * spacing - cellSize / 2.0f;
                const float width = cellSize;
                const float height = cellSize;

                rect(gridOffset + left, gridOffset + top, width, height);
            }
        }

        noFill();
        stroke(rgba(255));
        strokeWeight(2.0f);
        circle(mousePosition.x, mousePosition.y, affectionRadius);
    }
};

SketchSpec p5::createSpec()
{
    return {
        .sketch = [] {
            return std::make_unique<InteractiveGrid>();
        }
    };
}
