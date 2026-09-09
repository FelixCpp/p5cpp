#include <p5cpp/p5cpp.hpp>
#include <p5cpp_webcam/p5cpp_webcam.hpp>

#include <format>

using namespace p5;
using namespace p5::webcam;

struct WebcamExample : Sketch
{
    Capture video;
    Texture monaLisa = loadTexture("assets/Mona-Lisa.jpg").value();

    // clang-format off
    static constexpr std::string_view kAsciiRamps[] = {
        " .,_-~:;<=>'*+`^!?()ABCDEFGHIJKLMNOPQRSTUVWXYZ@#", // classic
        " .:-=+*#%@",                                        // simple
        " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$", // dense
        " 01",                                                // matrix
    };
    static constexpr std::string_view kRampNames[] = { "classic", "simple", "dense", "matrix" };
    // clang-format on

    size_t cellSize = 8;
    size_t rampIndex = 0;
    bool colorize = true;
    bool invert = false;
    bool showHud = true;

    void setup() override
    {
        setWindowSize(1920, 1080);
        setWindowTitle("ASCII Cam");

        if (auto capture = openCapture(1, {.requestedWidth = 1920, .requestedHeight = 1080, .requestedFPS = 60.0f, .flipHorizontal = true})) {
            video = std::move(*capture);
        } else {
            error("No available webcam found");
            quit(1);
        }
    }

    void event(const WindowEvent& windowEvent) override
    {
        windowEvent.on(
            [this](const WindowEvent::KeyPress& e) {
                // Only layout-independent keys here: GLFW's Key enum is named after physical
                // US-QWERTY positions, so e.g. Escape is safe but letters/brackets are not
                // (on a German QWERTZ layout, Y/Z are physically swapped and '[' ']' aren't
                // direct keys at all, so Key::LeftBracket/RightBracket never fire for them).
                if (e.key == Key::Escape) {
                    quit(0);
                }
            },
            [this](const WindowEvent::CharInput& e) {
                // CharInput comes from GLFW's char callback, which resolves the codepoint the
                // OS keyboard layout actually produces - use it for printable shortcuts instead.
                switch (e.codepoint) {
                    case U'c': case U'C': colorize = !colorize; break;
                    case U'i': case U'I': invert = !invert; break;
                    case U'h': case U'H': showHud = !showHud; break;
                    case U'r': case U'R': rampIndex = (rampIndex + 1) % std::size(kAsciiRamps); break;
                    case U'[': cellSize = cellSize > 3 ? cellSize - 1 : cellSize; break;
                    case U']': cellSize = cellSize < 32 ? cellSize + 1 : cellSize; break;
                    default: break;
                }
            });
    }

    void draw() override
    {
        background(rgba(10, 10, 18));
        noStroke();

        video.update();

        const std::optional<ReadOnlyPixels> pixels = video.loadPixels();
        if (pixels.has_value()) {
            const std::string_view ramp = kAsciiRamps[rampIndex];
            const size_t cols = pixels->width / cellSize;
            const size_t rows = pixels->height / cellSize;

            textSize(static_cast<float>(cellSize));
            textAlign(TextAlignment::center);

            for (size_t y = 0; y < rows; ++y) {
                for (size_t x = 0; x < cols; ++x) {
                    const size_t pixelX = x * cellSize;
                    const size_t pixelY = y * cellSize;
                    const color_t sourceColor = pixels->get(static_cast<int32_t>(pixelX), static_cast<int32_t>(pixelY));
                    uint8_t brightness = getBrightness(sourceColor);
                    if (invert) {
                        brightness = static_cast<uint8_t>(255 - brightness);
                    }
                    const char asciiChar = mapBrightnessToAscii(brightness, ramp);

                    if (colorize) {
                        // boost saturation a bit so faint webcam colors still pop against the dark backdrop
                        fill(rgba(getRed(sourceColor), getGreen(sourceColor), getBlue(sourceColor)));
                    } else {
                        fill(rgba(brightness));
                    }

                    text(std::string(1, asciiChar), static_cast<float>(pixelX), static_cast<float>(pixelY));
                }
            }
        } else {
            fill(rgba(255));
            textAlign(TextAlignment::center);
            textSize(24.0f);
            text("Waiting for camera frame...", static_cast<float>(getWindowSize().x) / 2.0f, static_cast<float>(getWindowSize().y) / 2.0f);
        }

        if (showHud) {
            drawHud();
        }
    }

    void drawHud()
    {
        fill(rgba(255, 220));
        textAlign(TextAlignment::topLeft);
        textSize(16.0f);
        text(std::format("ramp: {} (R)   cell: {}px ([/])   color: {} (C)   invert: {} (I)   hud: H",
                          kRampNames[rampIndex], cellSize, colorize ? "on" : "off", invert ? "on" : "off"),
             12.0f, 12.0f);
    }

    inline char mapBrightnessToAscii(uint8_t brightness, std::string_view ramp)
    {
        const size_t index = static_cast<size_t>(brightness / 255.0 * (ramp.size() - 1));
        return ramp[index];
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
