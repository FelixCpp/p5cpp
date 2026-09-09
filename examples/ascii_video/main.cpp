#include <p5cpp/p5cpp.hpp>
#include <p5cpp_webcam/p5cpp_webcam.hpp>

#include <format>

using namespace p5;
using namespace p5::webcam;

struct WebcamExample : Sketch
{
    Capture video;
    Texture monaLisa = loadTexture("assets/Mona-Lisa.jpg").value();

    void setup() override
    {
        setWindowSize(640, 480);

        if (auto capture = openCapture(0, {.requestedWidth = 640, .requestedHeight = 480, .requestedFPS = 30.0f, .flipHorizontal = true})) {
            video = std::move(*capture);
        } else {
            error("No available webcam found");
            quit(1);
        }
    }

    void draw() override
    {
        background(rgba(21, 21, 41));
        noStroke();
        fill(rgba(255));

        const auto start = std::chrono::high_resolution_clock::now();
        Pixels pixels = video.loadPixels();
        const auto end = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double, std::milli> elapsed = end - start;
        info("Frame load time: {:.2f} ms", elapsed.count());

        const size_t cellSize = 5;
        const size_t cols = pixels.width / cellSize;
        const size_t rows = pixels.height / cellSize;
        for (size_t y = 0; y < rows; ++y) {
            for (size_t x = 0; x < cols; ++x) {
                const size_t pixelX = x * cellSize;
                const size_t pixelY = y * cellSize;
                const color_t color = pixels.get(static_cast<int32_t>(pixelX), static_cast<int32_t>(pixelY));

                fill(color);
                rect(static_cast<float>(pixelX), static_cast<float>(pixelY), static_cast<float>(cellSize) - 1.0f, static_cast<float>(cellSize) - 1.0f);
            }
        }
    }

    inline char mapBrightnessToAscii(uint8_t brightness)
    {
        constexpr std::string_view asciiChars = " .,_-~:;<=>'*+`^!?()ABCDEFGHIJKLMNOPQRSTUVWXYZ@#";
        const size_t index = static_cast<size_t>(brightness / 255.0 * (asciiChars.size() - 1));
        return asciiChars[index];
    }
};

SketchSpec p5::createSpec()
{
    return {
        .plugins = [] {
            std::vector<std::unique_ptr<Plugin>> plugins;
            plugins.push_back(webcam::createWebcamPlugin(LogLevel::info));
            return plugins;
        },
        .sketch = [] {
            return std::make_unique<WebcamExample>();
        },
    };
}
