#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

using namespace p5;
using namespace p5::animation;

struct Button
{
    float2 position;
    float2 size;

    RepeatingTransitionComposite sizeTransition;

    explicit Button(float x, float y, float width, float height)
        : position {x, y},
          size {width, height},
          sizeTransition {
              repeating(
                  sequential({
                      waitUntil([this](float) {
                          return isBeingHovered();
                      }),
                      tween(0.3f, curves::easeInOutSine, [this, width, height](float progress) {
                          size.x = lerp(width, width + 10.0f, progress);
                          size.y = lerp(height, height + 10.0f, progress);
                      }),
                      waitUntil([this](float) {
                          return not isBeingHovered();
                      }),
                      tween(0.3f, curves::easeInOutSine, [this, width, height](float progress) {
                          size.x = lerp(width + 10.0f, width, progress);
                          size.y = lerp(height + 10.0f, height, progress);
                      }),
                  })
              )
          }
    {
    }

    bool isBeingHovered()
    {
        const float mx = static_cast<float>(getMouseX());
        const float my = static_cast<float>(getMouseY());
        const float left = position.x - size.x * 0.5f;
        const float top = position.y - size.y * 0.5f;
        const float right = position.x + size.x * 0.5f;
        const float bottom = position.y + size.y * 0.5f;

        return (mx >= left && mx <= right && my >= top && my <= bottom);
    }

    void update(float deltaTime)
    {
        sizeTransition.advance(deltaTime);
    }

    void show() const
    {
        withMatrix([this] {
            translate(position.x, position.y);
            fill(rgba(255, 255, 255));
            rect(-size.x * 0.5f, -size.y * 0.5f, size.x, size.y);
        });
    }
};

struct AnimationTest : Sketch
{
    std::vector<std::unique_ptr<Button>> buttons;

    void setup() override
    {
        setWindowSize(800, 800);

        for (size_t y = 0; y < 5; ++y) {
            buttons.push_back(std::make_unique<Button>(100.0f, 75.0f * (y + 1), 100.0f, 50.0f));
        }
    }

    void draw() override
    {
        background(rgba(31, 31, 51));

        for (std::unique_ptr<Button>& button : buttons) {
            button->update(getDeltaTime());
            button->show();
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
