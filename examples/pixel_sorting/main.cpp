#include <p5cpp/p5cpp.hpp>
#include <p5cpp_camera/p5cpp_camera.hpp>

using namespace p5;
using namespace p5::camera;

struct PixelSorting : Sketch
{
    inline static constexpr uint8_t thresold = 150;

    Texture texture = loadTexture("assets/Mountains.jpeg").value();
    bool isSorting = true;
    Pixels pixels = texture.loadPixels();
    int cursorX = 0;
    int cursorY = 0;

    void setup() override
    {
        setWindowSize(texture.size.x, texture.size.y);
    }

    void draw() override
    {

        if (isKeyPressed(Key::R)) {
            resetCamera();
        }

        background(rgba(31, 31, 51));

        const float mx = static_cast<float>(getMouseX());
        const float my = static_cast<float>(getMouseY());
        const float2 mousePos {mx, my};
        const size_t radius = 200;

        if (isSorting) {
            if (cursorX < pixels.width) {
                sortColumn(pixels, cursorX, 0, pixels.height);
                cursorX++;
            }

            if (cursorY < pixels.height) {
                sortRow(pixels, cursorY, 0, pixels.width);
                cursorY++;
            }

            texture.updatePixels(pixels);
            isSorting = cursorX < pixels.width || cursorY < pixels.height;
        }

        const float width = static_cast<float>(getWidth());
        const float height = static_cast<float>(getHeight());

        withCamera([&] {
            image(texture, 0.0f, 0.0f, width, height);
        });
    }

    void sortColumn(Pixels& pixels, size_t x, size_t initialY, size_t finalY)
    {
        size_t y = initialY;
        while (y < finalY) {
            while (y < finalY) {
                const color_t color = pixels.get(x, y);
                const uint8_t brightness = getBrightness(color);
                if (brightness > thresold) {
                    break;
                }

                ++y;
            }

            size_t startY = y;

            while (y < finalY) {
                const color_t color = pixels.get(x, y);
                const uint8_t brightness = getBrightness(color);
                if (brightness <= thresold) {
                    break;
                }

                ++y;
            }

            size_t endY = y - 1;
            if (startY < endY) {
                std::vector<color_t> colors;
                colors.reserve(endY - startY + 1);

                for (size_t i = startY; i <= endY; ++i) {
                    colors.push_back(pixels.get(x, i));
                }

                std::sort(colors.begin(), colors.end(), [](const color_t& a, const color_t& b) {
                    return getBrightness(a) < getBrightness(b);
                });

                for (size_t i = startY; i <= endY; ++i) {
                    pixels.set(x, i, colors[i - startY]);
                }
            }

            ++y;
        }
    }

    void sortRow(Pixels& pixels, size_t y, size_t initialX, size_t finalX)
    {
        size_t x = initialX;
        while (x < finalX) {
            while (x < finalX) {
                const color_t color = pixels.get(x, y);
                const uint8_t brightness = getBrightness(color);
                if (brightness > thresold) {
                    break;
                }

                ++x;
            }

            size_t startX = x;

            while (x < finalX) {
                const color_t color = pixels.get(x, y);
                const uint8_t brightness = getBrightness(color);
                if (brightness <= thresold) {
                    break;
                }

                ++x;
            }

            size_t endX = x - 1;
            if (startX < endX) {
                std::vector<color_t> colors;
                colors.reserve(endX - startX + 1);

                for (size_t i = startX; i <= endX; ++i) {
                    colors.push_back(pixels.get(i, y));
                }

                std::sort(colors.begin(), colors.end(), [](const color_t& a, const color_t& b) {
                    return getBrightness(a) < getBrightness(b);
                });

                for (size_t i = startX; i <= endX; ++i) {
                    pixels.set(i, y, colors[i - startX]);
                }
            }

            ++x;
        }
    }
};

SketchSpec p5::createSpec()
{
    return {
        .plugins = [] {
            std::vector<std::unique_ptr<Plugin>> plugins;
            plugins.emplace_back(createCameraPlugin());
            return plugins;
        },
        .sketch = [] {
            return std::make_unique<PixelSorting>();
        }
    };
}
