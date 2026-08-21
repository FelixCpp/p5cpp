#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>
using namespace p5;

typedef struct
{
    float x;
    float y;
    float width;
    float height;
    size_t colorIndex;
    uint8_t alpha;
} Cell;

struct InteractiveText : Sketch
{
    size_t columns;
    size_t rows;
    size_t cellWidth;
    size_t cellHeight;

    std::vector<Cell> cells;
    std::shared_ptr<Texture> texture = loadTextureFromFile("interactive_text.png");

    inline static constexpr color_t colorPalette[] = {
        0xabcd5eFF,
        0x14976bFF,
        0x2b67afFF,
        0x62b6deFF,
        0xf589a3FF,
        0xef562fFF,
        0xfc8405FF,
        0xf9d531FF,
    };

    void setup() override
    {
        cellWidth = 4;
        cellHeight = 5;
        columns = texture->size.x / cellWidth;
        rows = texture->size.y / cellHeight;

        setWindowSize(columns * cellWidth, rows * cellHeight);

        Pixels pixels = loadPixels(*texture);
        for (size_t row = 0; row < rows; ++row) {
            for (size_t column = 0; column < columns; ++column) {
                const color_t pixelColor = getPixel(pixels, column * cellWidth, row * cellHeight);
                const bool isLetter = getBrightness(pixelColor) < 127;
                if (not isLetter) {
                    continue;
                }

                const size_t randomColorIndex = static_cast<size_t>(random(0, std::size(colorPalette)));
                const Cell cell {
                    .x = static_cast<float>(column * cellWidth),
                    .y = static_cast<float>(row * cellHeight),
                    .width = static_cast<float>(cellWidth),
                    .height = static_cast<float>(cellHeight),
                    .colorIndex = randomColorIndex,
                    .alpha = 0,
                };
                cells.push_back(cell);
            }
        }
    }

    void draw() override
    {
        background(rgba(0));

        const float2 mousePosition = float2 {.x = static_cast<float>(getMouseX()), .y = static_cast<float>(getMouseY())};

        for (size_t i = 0; i < cells.size(); ++i) {
            Cell& cell = cells[i];
            const float2 cellCenter = float2 {.x = cell.x + cell.width * 0.5f, .y = cell.y + cell.height * 0.5f};
            const float dist = distanceSquared(mousePosition, cellCenter);
            const float viewRadius = 100.0f;
            const bool isVisible = dist < (viewRadius * viewRadius);
            if (isVisible) {
                // cell.alpha = 255;
                cell.alpha = std::min(255, static_cast<int>(cell.alpha) + 10);

            } else {
                cell.alpha = std::max(0, static_cast<int>(cell.alpha) - 10);
            }

            const color_t cellColor = (colorPalette[cell.colorIndex] & 0xFFFFFF00) | static_cast<color_t>(cell.alpha);

            fill(cellColor);
            stroke(rgba(0));
            strokeWeight(2.0f);
            rect(cell.x, cell.y, cell.width, cell.height);
        }
    }
};

// struct InteractiveText : Sketch
// {
//     std::shared_ptr<Font> font = loadFontFromFile("Arial Rounded Bold.ttf", 1024 * 3, 1024 * 3, 128 * 3);
//
//     void setup() override
//     {
//         rect2f bounds = textBounds(*font, 320.0f, "BIGGER");
//         setWindowSize(bounds.width, bounds.height);
//     }
//
//     void draw() override
//     {
//         background(rgba(255));
//
//         fill(rgba(0));
//         noStroke();
//         textFont(font);
//         textSize(320.0f);
//         textAlign(TextAlignment::center);
//         text("BIGGER", getWidth() * 0.5f, getHeight() * 0.5f, getWidth(), getHeight());
//
//         if (isKeyPressed(Key::Space)) {
//             Pixels pixels = loadPixels();
//             std::unique_ptr<Texture> texture = loadTextureFromMemory(pixels.width, pixels.height, {}, TexturePixelFormat::rgba8);
//             updatePixels(*texture, pixels);
//             saveTextureToFileAsPNG(*texture, "interactive_text.png");
//         }
//     }
// };

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<InteractiveText>();
}
