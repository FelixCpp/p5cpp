#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

using namespace p5;
using namespace p5::animation;

struct LightedPixels : Sketch
{
    std::vector<color_t> painting;

    void setup() override
    {
        const int width = 400;
        const int height = 400;

        setWindowSize(width, height);

        painting.resize(width * height, rgba(0));
        for (size_t y = 0; y < height; ++y) {
            for (size_t x = 0; x < width; ++x) {
                const uint8_t red = static_cast<uint8_t>(random(0.0f, 255.0f));
                const uint8_t green = static_cast<uint8_t>(random(0.0f, 255.0f));
                const uint8_t blue = static_cast<uint8_t>(random(0.0f, 255.0f));
                painting[y * width + x] = rgba(red, green, blue);
            }
        }
    }

    void draw() override
    {
        static constexpr float lightRadius = 100.0f;
        const float2 mouse = {.x = static_cast<float>(getMouseX()), .y = static_cast<float>(getMouseY())};

        background(rgba(0));

        Pixels pixels = loadPixels();
        for (size_t y = 0; y < 400; ++y) {
            for (size_t x = 0; x < 400; ++x) {
                const float2 pixelPos = {.x = static_cast<float>(x), .y = static_cast<float>(y)};
                const float distance = distanceSquared(pixelPos, mouse);
                const float lightIntensity = easeInOutElastic(constrain(1.0f - distance / (lightRadius * lightRadius), 0.0f, 1.0f));

                const color_t originalColor = painting[y * 400 + x];
                const color_t newColor = rgba(getRed(originalColor) * lightIntensity, getGreen(originalColor) * lightIntensity, getBlue(originalColor) * lightIntensity);
                setPixel(pixels, x, y, newColor);
            }
        }
        updatePixels(pixels);
    }
};

SketchSpec p5::createSpec()
{
    return {
        .sketch = [] {
            return std::make_unique<LightedPixels>();
        }
    };
}
