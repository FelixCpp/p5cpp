#include <p5cpp/p5cpp.hpp>
#include <p5cpp/p5cpp_animation.hpp>

using namespace p5;

struct EffectsSketch : public Sketch
{
    timeline<float> xMovement = createTimeline(
        100.0f,
        {
            {700.0f, 2.0f, easeLinear},
            {350.0f, 2.0f, easeInOutBounce},
        }
    );

    timeline<float> ballScale = createTimeline(
        1.0f,
        {
            {2.0f, 1.0f, easeInOutQuad},
            {1.5f, 1.0f, easeInOutQuad},
        }
    );

    spring<float> yMovement = createSpring(300.0f, 0.0f, 100.0f, 10.0f, 1.0f);

    void setup() override
    {
        setWindowSize(800, 600);

        loop(xMovement, LoopMode::pingpong);
        loop(ballScale, LoopMode::pingpong);
        loop(yMovement, LoopMode::pingpong);

        restart(xMovement);
        restart(yMovement);
        restart(ballScale);
    }

    void draw() override
    {
        advance(xMovement, getDeltaTime());
        advance(ballScale, getDeltaTime());
        advance(yMovement, getDeltaTime());

        background(rgba(21));

        fill(rgba(255));
        noStroke();

        withMatrix([this] {
            translate(value(xMovement), value(yMovement));
            scale(value(ballScale), value(ballScale));
            circle(0, 0, 50);
        });
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<EffectsSketch>();
}
