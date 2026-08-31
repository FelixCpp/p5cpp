#include <cassert>
#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

using namespace p5;
using namespace p5::animation;

auto yoyo(float duration, Curve curve, auto f)
{
    return sequential(
        tween(duration, curve, [f](float progress) {
            f(progress);
        }),
        tween(duration, reverse(curve), [f](float progress) {
            f(progress);
        })
    );
}

struct AnimationTest : Sketch
{
    float x = 200.0f;
    float y = 200.0f;
    float scl = 1.0f;

    sequential_transition_chain chain = sequential(
        wait_until([this]() {
            return isMouseButtonPressed(MouseButton::Left);
        }),
        repeat(
            [this] {
                return yoyo(1.0f, curves::easeInOutBack, [this](float progress) {
                    scl = lerp(1.0f, 2.0f, progress);
                });
            },
            2
        )
    );

    void setup() override
    {
        setWindowSize(1280, 720);
    }

    void draw() override
    {
        chain.advance(getDeltaTime());

        background(rgba(31, 31, 51));
        translate(x, y);
        scale(scl, scl);

        if (chain.get_transition_state() == transition_state::waiting) {
            fill(rgba(255, 0, 0));
        } else {
            fill(rgba(255));
        }

        noStroke();
        circle(0.0f, 0.0f, 50.0f);
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
