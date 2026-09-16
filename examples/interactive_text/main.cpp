#include <p5cpp/p5cpp.hpp>

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

static Texture createTexture(std::string_view text)
{
    const Font font = loadFont("fonts/Arial Rounded Bold.ttf", 1024 * 3, 1024 * 3, 128 * 3).value();
    const rect2f bounds = textBounds(text, {.font = font, .size = 320.0f});
    const Graphics graphics = createGraphics(bounds.width, bounds.height).value();

    withGraphics(graphics, [&]() {
        background(rgba(255));
        fill(rgba(0));
        noStroke();
        textFont(font);
        textSize(320.0f);
        textAlign(TextAlignment::center);
        p5::text(text, bounds.width * 0.5f, bounds.height * 0.5f, bounds.width, bounds.height);
    });

    return graphics.colorTexture;
}

struct InteractiveText : Sketch
{
    size_t columns;
    size_t rows;
    size_t cellWidth;
    size_t cellHeight;

    std::vector<Cell> cells;
    Texture texture = createTexture("Felix");

    inline static constexpr color_t colorPalette[] = {
        rgba(0xab, 0xcd, 0x5e),
        rgba(0x14, 0x97, 0x6b),
        rgba(0x2b, 0x67, 0xaf),
        rgba(0x62, 0xb6, 0xde),
        rgba(0xf5, 0x89, 0xa3),
        rgba(0xef, 0x56, 0x2f),
        rgba(0xfc, 0x84, 0x05),
        rgba(0xf9, 0xd5, 0x31),
    };

    void setup() override
    {
        cellWidth = 5;
        cellHeight = 5;
        columns = texture.size.x / cellWidth;
        rows = texture.size.y / cellHeight;

        setWindowSize(columns * cellWidth, rows * cellHeight);

        Pixels pixels = texture.loadPixels();
        for (size_t row = 0; row < rows; ++row) {
            for (size_t column = 0; column < columns; ++column) {
                const color_t pixelColor = pixels.get(column * cellWidth, row * cellHeight);
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
                cell.alpha = std::min(255, static_cast<int>(cell.alpha) + 10);
            } else {
                cell.alpha = std::max(0, static_cast<int>(cell.alpha) - 10);
            }

            const color_t cellColor = withAlpha(colorPalette[cell.colorIndex], cell.alpha);

            fill(cellColor);
            stroke(rgba(0));
            strokeWeight(2.0f);
            rect(cell.x, cell.y, cell.width, cell.height);
        }
    }
};

SketchSpec p5::createSpec()
{
    return {
        .sketch = [] {
            return std::make_unique<InteractiveText>();
        }
    };
}
