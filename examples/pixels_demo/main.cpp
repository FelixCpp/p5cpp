#include <p5cpp/p5cpp.hpp>

using namespace p5;

// Verifies the byte<->color_t roundtrip that loadPixels()/updatePixels() rely on, using a
// small offscreen Texture with known content (independent of anything drawn on screen).
static void verifyTexturePixelsRoundtrip()
{
    const std::array<uint8_t, 16> bytes {
        255, 0, 0, 255, // red
        0, 255, 0, 128, // green, half alpha
        0, 0, 255, 0, // blue, transparent
        10, 20, 30, 40, // arbitrary
    };
    std::unique_ptr<Texture> texture = loadTextureFromMemory(2, 2, bytes);

    Pixels pixels = loadPixels(*texture);
    const bool readOk = pixels.width == 2 and pixels.height == 2
        and getPixel(pixels, 0, 0) == rgba(255, 0, 0, 255)
        and getPixel(pixels, 1, 0) == rgba(0, 255, 0, 128)
        and getPixel(pixels, 0, 1) == rgba(0, 0, 255, 0)
        and getPixel(pixels, 1, 1) == rgba(10, 20, 30, 40);
    info("loadPixels(texture) byte<->color_t conversion: {}", readOk ? "OK" : "FAILED");

    setPixel(pixels, 0, 0, rgba(1, 2, 3, 4));
    updatePixels(*texture, pixels);
    const Pixels reloaded = loadPixels(*texture);
    const bool writeOk = getPixel(reloaded, 0, 0) == rgba(1, 2, 3, 4) and getPixel(reloaded, 1, 1) == rgba(10, 20, 30, 40);
    info("updatePixels(texture, pixels) roundtrip: {}", writeOk ? "OK" : "FAILED");
}

struct PixelsDemoSketch : public Sketch
{
    void setup() override
    {
        setWindowSize(400, 400);
        verifyTexturePixelsRoundtrip();
    }

    void draw() override
    {
        background(rgba(25));
        noStroke();
        fill(rgba(0, 120, 255));
        circle(200, 200, 150);

        // Read back the currently active (default) framebuffer and paint a red square directly
        // into the pixel buffer, bypassing draw calls entirely, then push it back to the GPU.
        Pixels pixels = loadPixels();
        for (int32_t y = 50; y < 100; ++y) {
            for (int32_t x = 50; x < 100; ++x) {
                setPixel(pixels, x, y, rgba(255, 0, 0));
            }
        }
        updatePixels(pixels);
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<PixelsDemoSketch>();
}
