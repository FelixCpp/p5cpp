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

color_t lerpColor(const color_t a, const color_t b, float t)
{
    const uint8_t red = static_cast<uint8_t>(lerp(static_cast<float>(getRed(a)), static_cast<float>(getRed(b)), t));
    const uint8_t green = static_cast<uint8_t>(lerp(static_cast<float>(getGreen(a)), static_cast<float>(getGreen(b)), t));
    const uint8_t blue = static_cast<uint8_t>(lerp(static_cast<float>(getBlue(a)), static_cast<float>(getBlue(b)), t));
    const uint8_t alpha = static_cast<uint8_t>(lerp(static_cast<float>(getAlpha(a)), static_cast<float>(getAlpha(b)), t));

    return rgba(red, green, blue, alpha);
}

struct AnimationTest : Sketch
{
    float x = 200.0f;
    float y = 200.0f;
    float scl = 1.0f;

    animation_controller controller;
    color_t color = rgba(255);

    sequential_transition_chain animateX()
    {
        return sequential(
            repeat(
                [this] {
                    return yoyo(2.0f, curves::easeInOutBack, [this](float progress) {
                        x = lerp(200.0f, 400.0f, progress);
                    });
                },
                2
            ),
            wait_for(0.5f),
            repeat(
                [this] {
                    return yoyo(2.0f, curves::easeInOutBack, [this](float progress) {
                        y = lerp(200.0f, 400.0f, progress);
                    });
                },
                2
            )
        );
    }

    void setup() override
    {
        setWindowSize(1280, 720);
    }

    void draw() override
    {
        controller.advance(getDeltaTime());

        if (isKeyPressed(Key::X)) {
            controller.play(animateX());
        }

        background(rgba(31, 31, 51));
        translate(x, y);
        scale(scl, scl);

        // if (.get_transition_state() == transition_state::waiting) {
        //     fill(rgba(255, 0, 0));
        // } else {
        //     fill(rgba(255));
        // }

        noStroke();
        fill(color);
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
