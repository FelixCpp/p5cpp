#include <p5cpp/p5cpp.hpp>

using namespace p5;

struct VPainting : Sketch
{
    Texture monaLisa;

    size_t cellSize;
    size_t columns;
    size_t rows;

    float noiseOffsetX;
    float noiseOffsetY;
    std::vector<float> noiseValues;

    Graphics painting;

    static size_t getCellSize(size_t canvasWidth, size_t canvasHeight)
    {
        const size_t minCellSize = 10;
        const size_t maxCellSize = 25;

        const size_t cellSize = std::max(minCellSize, std::min(maxCellSize, canvasWidth / 100));
        return cellSize;
    }

    void setup() override
    {
        setWindowResizable(false);
        if (std::optional<Texture> texture = loadTexture("Mona-Lisa.jpg")) {
            monaLisa = std::move(*texture);
        } else {
            error("Failed to load Mona-Lisa.jpg texture.");
            quit(1);
        }

        const auto [textureWidth, textureHeight] = monaLisa.size;
        const float aspectRatio = static_cast<float>(textureWidth) / textureHeight;

        const size_t canvasHeight = 1000;
        const size_t canvasWidth = static_cast<size_t>(canvasHeight * aspectRatio);

        cellSize = getCellSize(canvasWidth, canvasHeight);
        columns = canvasWidth / cellSize;
        rows = canvasHeight / cellSize;
        noiseValues = generateNoiseValues(columns, rows);

        if (std::optional<Graphics> graphics = createGraphics(canvasWidth, canvasHeight)) {
            painting = std::move(*graphics);
        } else {
            error("Failed to create graphics for painting.");
            quit(1);
        }

        withGraphics(painting, [this] {
            drawPainting(monaLisa.loadPixels());
        });

        setWindowSize(canvasWidth, canvasHeight);
    }

    void drawPainting(const Pixels& pixels)
    {
        for (size_t y = 0; y < rows; ++y) {
            for (size_t x = 0; x < columns; ++x) {
                const int monaLisaPixelX = static_cast<int>((static_cast<float>(x) / static_cast<float>(columns)) * static_cast<float>(monaLisa.size.x));
                const int monaLisaPixelY = static_cast<int>((static_cast<float>(y) / static_cast<float>(rows)) * static_cast<float>(monaLisa.size.y));
                const color_t monaLisaPixelColor = pixels.get(monaLisaPixelX, monaLisaPixelY);
                const int screenPixelX = static_cast<int>(x * cellSize);
                const int screenPixelY = static_cast<int>(y * cellSize);
                const float cellCenterX = static_cast<float>(screenPixelX + static_cast<int>(cellSize) / 2);
                const float cellCenterY = static_cast<float>(screenPixelY + static_cast<int>(cellSize) / 2);

                const float noiseValue = noiseValues[y * columns + x];
                const bool isHorizontal = noiseValue > 0.5f;

                drawV(cellCenterX, cellCenterY, isHorizontal, monaLisaPixelColor);
            }
        }
    }

    void draw() override
    {
        background(rgba(21, 21, 31));
        const float windowCenterX = static_cast<float>(getWidth()) * 0.5f;
        const float windowCenterY = static_cast<float>(getHeight()) * 0.5f;
        const float paintingWidth = static_cast<float>(painting.size.x);
        const float paintingHeight = static_cast<float>(painting.size.y);
        noStroke();
        fill(rgba(0));
        rect(windowCenterX - paintingWidth * 0.5f, windowCenterY - paintingHeight * 0.5f, paintingWidth, paintingHeight);
        image(painting, windowCenterX - paintingWidth * 0.5f, windowCenterY - paintingHeight * 0.5f, paintingWidth, paintingHeight);
    }

    void drawV(const float cellCenterX, const float cellCenterY, const bool horizontal, const color_t color)
    {
        withMatrix([this, cellCenterX, cellCenterY, horizontal, color] {
            translate(cellCenterX, cellCenterY);
            if (horizontal) {
                rotate(radians(90.0f));
            }

            const float cellSpacing = 2.0f;
            const float halfCellSize = static_cast<float>(cellSize) * 0.5f - cellSpacing;
            const float strokeWeightValue = static_cast<float>(cellSize) * 0.1f;

            noFill();
            stroke(color);
            strokeWeight(strokeWeightValue);
            strokeCap(StrokeCap::round);
            strokeJoin(StrokeJoin::round);
            beginShape();
            vertex(-halfCellSize, -halfCellSize);
            vertex(0.0f, halfCellSize);
            vertex(halfCellSize, -halfCellSize);
            endShape();
        });
    }

    static std::vector<float> generateNoiseValues(size_t columns, size_t rows)
    {
        std::vector<float> values(columns * rows, 0.0f);
        float offsetY = 0.0f;
        for (size_t y = 0; y < rows; ++y) {
            float offsetX = 0.0f;
            for (size_t x = 0; x < columns; ++x) {
                const float noiseValue = noise(offsetX, offsetY);
                values[y * columns + x] = noiseValue;
                offsetX += 0.1f;
            }
            offsetY += 0.1f;
        }
        return values;
    }
};

SketchSpec p5::createSpec()
{
    return {
        .sketch = [] {
            return std::make_unique<VPainting>();
        }
    };
}
