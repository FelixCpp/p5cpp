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
        setWindowSize(1280, 720);

        const auto availableCameras = getAvailableCameras();
        if (availableCameras.empty()) {
            error("No webcam found.");
            quit(1);
            return;
        }

        targetWebcam = availableCameras[0];

        const auto webcamCapture = openCapture(
            targetWebcam.globalIndex,
            CaptureOptions {
                .requestedWidth = 960,
                .requestedHeight = 540,
                .requestedFPS = 30.0f,
            }
        );

        if (not webcamCapture.has_value()) {
            error("Failed to open the default webcam.");
            quit(1);
            return;
        }

        capture = std::move(webcamCapture).value();

        std::string resolutions;
        for (const WebcamResolution& resolution : capture.getSupportedResolutions()) {
            resolutions += std::format("{}x{} ", resolution.width, resolution.height);
        }

        info("Supported resolutions: {}", resolutions);
    }

    void draw() override
    {
        background(rgba(18, 18, 24));

        const Texture frame = capture.getTexture();
        if (frame.isValid()) {
            const float2 windowSize {static_cast<float>(getWindowSize().x), static_cast<float>(getWindowSize().y)};
            image(frame, 0.0f, 0.0f, windowSize.x, windowSize.y);
        }

        drawOverlay();
    }

    void drawOverlay()
    {
        noStroke();
        fill(rgba(255));
        textAlign(TextAlignment::topLeft);
        textSize(14.0f);
        text(std::format("{} @ {:.0f} fps", targetWebcam.name, capture.getFPS()), 16.0f, 16.0f);
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
