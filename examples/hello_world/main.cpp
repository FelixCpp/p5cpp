#include <p5cpp/p5cpp.hpp>
#include <p5cpp/p5cpp_animation.hpp>

using namespace p5;

struct EffectsSketch : public Sketch
{
    tween<float> tweenValue = createTween(100.0f, 700.0f, 2.0f, easeInOutQuad, LoopMode::pingpong);
    tween<float> tweenScale = createTween(1.0f, 2.0f, 2.0f, easeInOutBack, LoopMode::pingpong);
    tween<color_t> tweenColor = createTween(rgba(255, 0, 0), rgba(0, 0, 255), 2.0f, easeInOutQuad, LoopMode::pingpong);

    timeline<float> timelineValue = createTimeline(
        100.0f,
        {
            {200.0f, 1.0f, easeInOutQuad},
            {400.0f, 1.0f, easeInOutQuad},
            {600.0f, 1.0f, easeInOutQuad},
            {700.0f, 1.0f, easeInOutQuad},
        }
    );

    void setup() override
    {
        setWindowSize(800, 600);
    }

    void draw() override
    {
        if (isKeyPressed(Key::Space)) {
            if (isPlaying(tweenValue)) {
                pause(tweenValue);
                pause(tweenColor);
                pause(tweenScale);
            } else {
                resume(tweenValue);
                resume(tweenColor);
                resume(tweenScale);
            }
        }

        advance(tweenValue, getDeltaTime());
        advance(tweenColor, getDeltaTime());
        advance(tweenScale, getDeltaTime());

        background(rgba(25));

        pushMatrix();
        translate(value(tweenValue), 300.0f);
        scale(value(tweenScale), value(tweenScale));
        fill(value(tweenColor));
        stroke(255);
        strokeWeight(3.0f);
        ellipse(0.0f, 0.0f, 50.0f, 50.0f);
        popMatrix();
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<EffectsSketch>();
}
