#include <algorithm>
#include <p5cpp/p5cpp.hpp>

namespace
{
    using namespace p5cpp;

    struct PixelsDemoSketch : Sketch
    {
        inline static constexpr int W = 640;
        inline static constexpr int H = 640;
        inline static constexpr int GRID = 8;
        inline static constexpr float BRUSH_RADIUS = 90.0f;

        void setup() override
        {
            setWindowSize(W, H);
            setWindowTitle("loadPixels / updatePixels demo");
        }

        // Ordinary vector-graphics drawing — this is what loadPixels() will read back below.
        void drawColorGrid()
        {
            noStroke();
            const float cellWidth = static_cast<float>(W) / GRID;
            const float cellHeight = static_cast<float>(H) / GRID;

            for (int row = 0; row < GRID; ++row) {
                for (int col = 0; col < GRID; ++col) {
                    const float hue = remap(static_cast<float>(row * GRID + col), 0.0f, static_cast<float>(GRID * GRID), 0.0f, 360.0f);
                    fill(hsv(hue, 0.8f, 1.0f));
                    rect(static_cast<float>(col) * cellWidth, static_cast<float>(row) * cellHeight, cellWidth, cellHeight);
                }
            }
        }

        // x/y pixel access via get()/set() — inverts colors in a circular
        // brush that follows the mouse.
        void invertUnderMouse(Pixels& pixels) const
        {
            const int mouseXPos = getMouseX();
            const int mouseYPos = getMouseY();

            const int minX = std::max(0, static_cast<int>(static_cast<float>(mouseXPos) - BRUSH_RADIUS));
            const int maxX = std::min(static_cast<int>(pixels.width) - 1, static_cast<int>(static_cast<float>(mouseXPos) + BRUSH_RADIUS));
            const int minY = std::max(0, static_cast<int>(static_cast<float>(mouseYPos) - BRUSH_RADIUS));
            const int maxY = std::min(static_cast<int>(pixels.height) - 1, static_cast<int>(static_cast<float>(mouseYPos) + BRUSH_RADIUS));

            for (int y = minY; y <= maxY; ++y) {
                for (int x = minX; x <= maxX; ++x) {
                    const float dx = static_cast<float>(x - mouseXPos);
                    const float dy = static_cast<float>(y - mouseYPos);
                    if (dx * dx + dy * dy > BRUSH_RADIUS * BRUSH_RADIUS) {
                        continue;
                    }

                    const color_t original = get(pixels, static_cast<uint32_t>(x), static_cast<uint32_t>(y));
                    const color_t inverted = rgba(255 - red(original), 255 - green(original), 255 - blue(original), alpha(original));
                    set(pixels, static_cast<uint32_t>(x), static_cast<uint32_t>(y), inverted);
                }
            }
        }

        // Flat data access — darkens every 4th row across the whole canvas.
        void darkenScanlines(Pixels& pixels) const
        {
            const uint32_t width = pixels.width;
            const uint32_t height = pixels.height;
            color_t* raw = pixels.data.data();

            for (uint32_t y = 0; y < height; y += 4) {
                color_t* scanline = raw + static_cast<size_t>(y) * width;
                for (uint32_t x = 0; x < width; ++x) {
                    scanline[x] = darken(scanline[x], 0.35f);
                }
            }
        }

        void draw() override
        {
            drawColorGrid();

            Pixels pixels = loadPixels();
            invertUnderMouse(pixels);
            darkenScanlines(pixels);
            updatePixels(pixels);
        }
    };
} // namespace

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<PixelsDemoSketch>();
    }
} // namespace p5cpp
