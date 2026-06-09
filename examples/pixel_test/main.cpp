#include <p5.hpp>

#include <algorithm>

using namespace p5;

struct PixelTestSketch : Sketch
{
    void setup() override
    {
    }

    void draw() override
    {
        constexpr float maxDistance = 200.0f;

        Pixels pixels = loadPixels();

        for (int y = 0; y < pixels.height; ++y) {
            for (int x = 0; x < pixels.width; ++x) {
                int dx = x - mouseX;
                int dy = y - mouseY;
                float distanceSquared = static_cast<float>(dx * dx + dy * dy);
                float distance = std::sqrt(distanceSquared);
                float intensity = 1.0f - std::min(distance / maxDistance, 1.0f);

                const size_t idx = static_cast<size_t>(y) * pixels.width + x;
                const uint8_t r = randomInt(255) * intensity;
                const uint8_t g = randomInt(255) * intensity;
                const uint8_t b = randomInt(255) * intensity;
                const uint8_t a = 255;
                // const uint8_t r = static_cast<uint8_t>((x / static_cast<float>(pixels.width)) * 255);
                // const uint8_t g = static_cast<uint8_t>((y / static_cast<float>(pixels.height)) * 255);
                // const uint8_t b = 128;
                // const uint8_t a = 255;
                pixels.colors[idx] = rgba(r, g, b, a);
            }
        }

        updatePixels(pixels);
    }
};

namespace p5
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<PixelTestSketch>();
    }
} // namespace p5
