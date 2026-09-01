#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

using namespace p5;
using namespace p5::animation;

struct AnimationTest : Sketch
{
    float x = 200.0f;
    float y = 200.0f;
    float scl = 1.0f;

    SequentialTransitionComposite seq = sequential({
            waitFor(1.0f),
            parallel({
                    tween(2.0f, curves::easeInOutSine, [this](float progress) {
                            x = lerp(200.0f, 400.0f, progress);
                            }),
                    tween(2.0f, curves::easeInOutSine, [this](float progress) {
                            y = lerp(200.0f, 400.0f, progress);
                            }),
                    }),
    });

    void setup() override
    {
        setWindowSize(1280, 720);
    }

    void draw() override
    {
        seq.advance(getDeltaTime());

        background(rgba(31, 31, 51));
        translate(x, y);
        scale(scl, scl);

        noStroke();
        fill(rgba(255, 0, 0));
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
