#include <p5cpp/p5cpp.hpp>
#include <p5cpp_gif/p5cpp_gif.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

using namespace p5;

// Demonstrates p5cpp_gif's saveGif() (p5cpp's equivalent of p5.js's saveGif()). Starts a 2s/30fps
// recording automatically on launch -- so this example doubles as a smoke test, just run it and
// check gif_recorder_example.gif afterwards -- and also lets you press S to trigger further
// recordings interactively, the way a p5.js sketch normally would.
struct GifRecorderExample : Sketch
{
    float x = 60.0f;
    float velocityX = 240.0f;

    void setup() override
    {
        // Deliberately not calling setWindowSize() here: it only takes effect once the resulting
        // WindowResize event is processed on a later frame, so calling saveGif() in the same setup()
        // right after it would capture the (still stale) old framebuffer size.
        gif::saveGif("gif_recorder_example.gif", 2.0f, 30);
    }

    void draw() override
    {
        background(rgba(15, 15, 25));

        const float deltaTime = static_cast<float>(getDeltaTime());
        const float w = getWidth();
        const float h = getHeight();

        x += velocityX * deltaTime;
        if (x < 30.0f or x > w - 30.0f) {
            velocityX = -velocityX;
            x = std::clamp(x, 30.0f, w - 30.0f);
        }

        // Cycle the fill color over time so a wrong RGBA byte order (see the p5cpp_gif bugfixes) would
        // show up as an obviously wrong hue instead of hiding in a single flat color.
        const float t = static_cast<float>(getGlobalTime());
        const int red = static_cast<int>(128.0f + 127.0f * std::sin(t * 2.0f));
        const int green = static_cast<int>(128.0f + 127.0f * std::sin(t * 2.0f + 2.094f));
        const int blue = static_cast<int>(128.0f + 127.0f * std::sin(t * 2.0f + 4.188f));

        noStroke();
        fill(rgba(red, green, blue));
        circle(x, h * 0.5f, 40.0f);

        if (isKeyPressed(Key::S)) {
            gif::saveGif("gif_recorder_example.gif", 2.0f, 30);
        }

        // Keep this a self-contained smoke test: close automatically once the auto-started recording
        // above should have finished.
        if (getGlobalTime() > 3.0) {
            quit();
        }
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
