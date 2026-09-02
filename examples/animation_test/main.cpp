#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

using namespace p5;
using namespace p5::animation;

struct Button
{
    float2 position;
    float2 baseSize;

    SpringTransition sizeBoost = spring(140.0f, 10.0f, 0.0f, 0.0f);

    explicit Button(float x, float y, float width, float height)
        : position {x, y},
          baseSize {width, height}
    {
    }

    float2 currentSize() const
    {
        const float boost = sizeBoost.value() * 10.0f;
        return {baseSize.x + boost, baseSize.y + boost};
    }

    bool isBeingHovered() const
    {
        const float mx = static_cast<float>(getMouseX());
        const float my = static_cast<float>(getMouseY());
        const float2 size = currentSize();
        const float left = position.x - size.x * 0.5f;
        const float top = position.y - size.y * 0.5f;
        const float right = position.x + size.x * 0.5f;
        const float bottom = position.y + size.y * 0.5f;

        return (mx >= left && mx <= right && my >= top && my <= bottom);
    }

    void update(float deltaTime)
    {
        sizeBoost.retarget(isBeingHovered() ? 1.0f : 0.0f);
        sizeBoost.advance(deltaTime);
    }

    void show() const
    {
        const float2 size = currentSize();
        withMatrix([this, size] {
            translate(position.x, position.y);
            fill(rgba(255, 255, 255));
            rect(-size.x * 0.5f, -size.y * 0.5f, size.x, size.y);
        });
    }
};

struct AnimationTest : Sketch
{
    std::vector<Button> buttons;

    void setup() override
    {
        setWindowSize(800, 800);

        for (size_t y = 0; y < 5; ++y) {
            buttons.emplace_back(100.0f, 75.0f * (y + 1), 100.0f, 50.0f);
        }
    }

    void draw() override
    {
        background(rgba(31, 31, 51));

        for (Button& button : buttons) {
            button.update(getDeltaTime());
            button.show();
        }
    }
};

SketchSpec p5::createSpec()
{
    return {
        .sketch = [] {
            return std::make_unique<AnimationTest>();
        }
    };
}
