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
    std::optional<GifRecording> recording;

    RepeatingTransitionComposite seq = repeating(sequential({
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
        if (isMouseButtonPressed(MouseButton::Left) and not recording.has_value()) {
            recording = recordGif("pretty_animation.gif", recordForSeconds(4.0f), {.framesPerSecond = 15.0f});
        }

        if (recording.has_value() and recording->isActive() and isMouseButtonPressed(MouseButton::Right)) {
            recording->cancel();
        }

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
