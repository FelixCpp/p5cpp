#include <p5cpp/p5cpp.hpp>
#include <p5cpp_gif/p5cpp_gif.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

using namespace p5;
using namespace p5::gif;
using namespace p5::animation;

struct GifRecorderExample : Sketch
{
    float x = 100.0f;
    float y = 100.0f;

    // ValueTweenTransition<float> f = valueTween(0.0f, 400.0f, 2.0f, curves::easeInOutQuad, [this](const float& value) {
    //     x = value;
    // });

    RepeatingTransitionComposite seq = repeating(sequential({
        waitUntil([](float) {
            return isMouseButtonPressed(MouseButton::Left);
        }),
        intercept([this]() {
            // Use 15fps instead of 30 for smoother playback.
            // Higher fps = more glReadPixels calls per second → more GPU stalls.
            // At 15fps on a 400x400 canvas: ~2 blocking readbacks/sec — much less stutter.
            saveGif("pretty_animation.gif", 4.0f, 15);
        }),
        tween(1.0f, curves::easeInOutSine, [this](float progress) {
            x = lerp(100.0f, 300.0f, progress);
        }),
        tween(1.0f, curves::easeInOutSine, [this](float progress) {
            y = lerp(100.0f, 300.0f, progress);
        }),
        tween(1.0f, curves::easeInOutSine, [this](float progress) {
            x = lerp(300.0f, 100.0f, progress);
        }),
        tween(1.0f, curves::easeInOutSine, [this](float progress) {
            y = lerp(300.0f, 100.0f, progress);
        }),
    }));

    void setup() override
    {
        setWindowSize(400, 400);
    }

    void draw() override
    {
        // if (isKeyPressed(Key::Space)) {
        //     // Lower frame rate (e.g. 15) reduces stutter significantly for large canvases or long recordings.
        //     saveGif("pretty_animation.gif", 3.0f, 15);
        // }

        seq.advance(getDeltaTime());

        background(rgba(15, 15, 25));
        noStroke();
        fill(rgba(255, 100, 100));
        circle(x, y, 50);
    }
};

SketchSpec p5::createSpec()
{
    return {
        .plugins = [] {
            std::vector<std::unique_ptr<Plugin>> plugins;
            plugins.push_back(gif::createGIFRecorderPlugin());
            return plugins;
        },
        .sketch = [] {
            return std::make_unique<GifRecorderExample>();
        },
    };
}
