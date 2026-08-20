#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>
using namespace p5;

inline static constexpr float BLOCK_SIZE = 10.0f;
inline static constexpr float BLOCK_SPACING = 4.0f;

struct Block
{
    float2 position;
    float angle;
    color_t color;

    explicit Block(float2 pos)
        : position {pos},
          angle {0.0f},
          color(rgba(70))
    {
    }

    void reactToTarget(const float2& target, bool hasTargetMoved, float reactionRadius)
    {
        if (hasTargetMoved) {
            const float dist = distance(position, target);
            if (dist < reactionRadius) {
                angle += 1.0f;
                color = rgba(255);
            }
        }

        if (angle > 0.0f and angle < 90.0f) {
            angle += 1.0f;

            if (color > rgba(70)) {
                color -= rgba(3, 0);
            }
        } else {
            angle = 0.0f;
            color = rgba(70);
        }
    }

    void update(float deltaTime)
    {
    }

    void show() const
    {
        withMatrix([&]() {
            translate(position.x, position.y);
            rotate(radians(angle));
            stroke(color);
            strokeWeight(2.0f);
            noFill();
            if (angle > 0.0f and angle < 45.0f) {
                const float blockSize = BLOCK_SIZE - BLOCK_SPACING;
                rect(-blockSize / 2, -blockSize / 2, blockSize, blockSize);
            } else {
                const float offset = BLOCK_SPACING;
                const float margin = -BLOCK_SIZE / 2;
                line(margin + offset * 0.5f, margin + offset * 0.5f, margin + BLOCK_SIZE - offset * 0.5f, margin + BLOCK_SIZE - offset * 0.5f);
                line(margin + BLOCK_SIZE - offset * 0.5f, margin + offset * 0.5f, margin + offset * 0.5f, margin + BLOCK_SIZE - offset * 0.5f);
            }
        });
    }
};

struct InteractiveDisplay : Sketch
{
    size_t columns;
    size_t rows;
    std::vector<Block> blocks;

    void setup() override
    {
        setWindowSize(400, 400);
        strokeCap(StrokeCap::butt);
        strokeJoin(StrokeJoin::bevel);

        columns = getWidth() / BLOCK_SIZE;
        rows = getHeight() / BLOCK_SIZE;
        for (size_t y = 0; y < rows; ++y) {
            for (size_t x = 0; x < columns; ++x) {
                float2 pos {
                    .x = BLOCK_SIZE * 0.5f + static_cast<float>(x * BLOCK_SIZE),
                    .y = BLOCK_SIZE * 0.5f + static_cast<float>(y * BLOCK_SIZE)
                };
                blocks.emplace_back(pos);
            }
        }
    }

    void draw() override
    {
        background(rgba(0));

        const float2 mousePosition {static_cast<float>(getMouseX()), static_cast<float>(getMouseY())};
        const bool hasMouseMoved = getMouseDeltaX() != 0.0 or getMouseDeltaY() != 0.0;

        for (size_t i = 0; i < blocks.size(); ++i) {
            Block& block = blocks[i];

            block.reactToTarget(mousePosition, hasMouseMoved, 15.0f);
            block.update(getDeltaTime());
            block.show();
        }
    }

    void centeredRect(float left, float top, float width, float height)
    {
        rect(left - width / 2, top - height / 2, width, height);
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<InteractiveDisplay>();
}
