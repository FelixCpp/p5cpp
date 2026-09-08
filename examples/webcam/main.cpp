#include <p5cpp/p5cpp.hpp>
#include <p5cpp_webcam/p5cpp_webcam.hpp>
#include <p5cpp_camera/p5cpp_camera.hpp>

#include <format>

using namespace p5;
using namespace p5::webcam;
using namespace p5::camera;

struct WebcamExample : Sketch
{
    Capture capture;
    Webcam targetWebcam;

    void setup() override
    {
        setWindowSize(640, 480);

        const CaptureOptions options {
            .requestedWidth = 640,
            .requestedHeight = 480,
            .requestedFPS = 30.0f,
            .flipHorizontal = false,
        };

        if (getAvailableCameras().empty()) {
            error("No webcams available.");
            return;
        }

        std::optional<Capture> openedCapture = openCapture(1, options);
        if (not openedCapture.has_value()) {
            error("Failed to open the default webcam.");
            return;
        }

        capture = std::move(openedCapture).value();
    }

    void draw() override
    {
        background(rgba(18, 18, 24));
        repaint();
    }

    void repaint()
    {
        Texture texture = capture.getTexture();
        if (not texture.isValid()) {
            return;
        }

        Pixels pixels = texture.loadPixels();
        if (pixels.data.empty()) {
            return;
        }

        int scl = 5;
        int w = 640 / scl;
        int h = 480 / scl;

        beginCamera();
        noStroke();
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const int mappedX = static_cast<int>(static_cast<float>(x) / w * pixels.width);
                const int mappedY = static_cast<int>(static_cast<float>(y) / h * pixels.height);
                const color_t c = pixels.get(mappedX, mappedY);
                const uint8_t brightness = getBrightness(c);
                const float radius = map(brightness, 0.0f, 255.0f, 0.0f, scl);
                const float positionX = static_cast<float>(x * scl) + scl * 0.5f;
                const float positionY = static_cast<float>(y * scl) + scl * 0.5f;
                const float2 cameraPos = camera::worldToScreen(positionX, positionY);

                fill(c);
                circle(cameraPos.x, cameraPos.y, radius);
            }
        }

        stroke(rgba(255));
        noFill();
        strokeWeight(1.0f);
        const float2 topLeft = camera::worldToScreen(0.0f, 0.0f);
        const float2 bottomRight = camera::worldToScreen(static_cast<float>(w * scl), static_cast<float>(h * scl));
        rect(topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y);
        endCamera();
    }
};

SketchSpec p5::createSpec()
{
    return {
        .plugins = [] {
            std::vector<std::unique_ptr<Plugin>> plugins;
            plugins.push_back(webcam::createWebcamPlugin());
            plugins.push_back(camera::createCameraPlugin());
            return plugins;
        },
        .sketch = [] {
            return std::make_unique<WebcamExample>();
        },
    };
}
