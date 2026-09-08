#include <p5cpp/p5cpp.hpp>
#include <p5cpp_webcam/p5cpp_webcam.hpp>

#include <format>

using namespace p5;
using namespace p5::webcam;

struct WebcamExample : Sketch
{
    Capture capture;
    Webcam targetWebcam;

    void setup() override
    {
        setWindowSize(640, 480);
        capture = openCapture(
                      0, {
                             .requestedWidth = 640,
                             .requestedHeight = 480,
                             .requestedFPS = 30.0f,
                             .flipHorizontal = true,
                         }
        ).value();
    }

    void draw() override
    {
        background(rgba(18, 18, 24));

        const Texture frame = capture.getTexture();
        if (frame.isValid()) {
            const float2 windowSize {static_cast<float>(getWindowSize().x), static_cast<float>(getWindowSize().y)};
            image(frame, 0.0f, 0.0f, windowSize.x, windowSize.y);
        }
    }
};

SketchSpec p5::createSpec()
{
    return {
        .plugins = [] {
            std::vector<std::unique_ptr<Plugin>> plugins;
            plugins.push_back(webcam::createWebcamPlugin());
            return plugins;
        },
        .sketch = [] {
            return std::make_unique<WebcamExample>();
        },
    };
}
