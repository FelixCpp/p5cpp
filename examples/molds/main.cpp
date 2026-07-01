#include <memory>
#include <p5cpp/p5cpp.hpp>

#include <algorithm>
#include <vector>

using namespace p5cpp;

struct PixelGrid
{
    size_t columns;
    size_t rows;
    std::vector<uint8_t> pixels;
    std::unique_ptr<Texture> texture;

    explicit PixelGrid(size_t columns, size_t rows)
        : columns(columns),
          rows(rows),
          pixels(columns * rows * 4, 0),
          texture(createTexture(static_cast<uint32_t>(columns), static_cast<uint32_t>(rows), pixels))
    {
    }

    uint2 getCellAtPosition(float x, float y) const
    {
        const uint32_t column = static_cast<uint32_t>(x / (static_cast<float>(getWidth()) / columns));
        const uint32_t row = static_cast<uint32_t>(y / (static_cast<float>(getHeight()) / rows));
        return {column, row};
    }

    uint8_t getAtCell(int32_t column, int32_t row) const
    {
        // Wrap statt clamp
        column = ((column % static_cast<int32_t>(columns)) + columns) % columns;
        row = ((row % static_cast<int32_t>(rows)) + rows) % rows;
        return pixels[(row * columns + column) * 4];
    }

    void markAtPosition(float x, float y, uint8_t value)
    {
        const uint2 cell = getCellAtPosition(x, y);
        markAtCell(cell.x, cell.y, value);
    }

    uint8_t getAtPosition(float x, float y) const
    {
        const uint2 cell = getCellAtPosition(x, y);
        return getAtCell(cell.x, cell.y);
    }

    void markAtCell(uint32_t column, uint32_t row, uint8_t value)
    {
        if (column >= columns || row >= rows) return;
        const size_t idx = (row * columns + column) * 4;
        // Addieren statt überschreiben, mit Clamp auf 255
        pixels[idx + 0] = static_cast<uint8_t>(std::min(255, pixels[idx + 0] + value));
        pixels[idx + 1] = static_cast<uint8_t>(std::min(255, pixels[idx + 1] + value));
        pixels[idx + 2] = static_cast<uint8_t>(std::min(255, pixels[idx + 2] + value));
        pixels[idx + 3] = 255;
    }

    void update(float deltaTime)
    {
        // Diffusion + Decay in einem Pass
        std::vector<uint8_t> next(pixels.size(), 0);

        for (size_t row = 1; row < rows - 1; ++row) {
            for (size_t column = 1; column < columns - 1; ++column) {
                // 3x3 Blur
                int sum = 0;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx)
                        sum += getAtCell(column + dx, row + dy);

                const uint8_t diffused = static_cast<uint8_t>(sum / 9);
                const uint8_t decayed = static_cast<uint8_t>(diffused * 0.9995f); // fester Wert, unabhängig von deltaTime

                const size_t idx = (row * columns + column) * 4;
                next[idx + 0] = decayed;
                next[idx + 1] = decayed;
                next[idx + 2] = decayed;
                next[idx + 3] = 255;
            }
        }

        pixels = std::move(next);
    }

    void show()
    {
        texture->update(pixels);
        image(texture.get(), 0, 0, getWidth(), getHeight());
    }
};

struct Sensors
{
    float2 center;
    float2 left;
    float2 right;
};

struct Mold
{
    float2 position;
    float2 direction;
    float travelSpeed;
    float radius;

    inline static const float SENSING_DISTANCE = 100.0f;
    inline static const float SENSING_ANGLE = radians(45.0f);

    void update(PixelGrid& pixelGrid, float deltaTime)
    {
        position += direction * deltaTime;
        position.x = fmod(position.x + getWidth(), getWidth());
        position.y = fmod(position.y + getHeight(), getHeight());

        pixelGrid.markAtPosition(position.x, position.y, 255);

        const Sensors sensors = getSensors(SENSING_DISTANCE, SENSING_ANGLE);
        const int brightnessCenter = pixelGrid.getAtPosition(sensors.center.x, sensors.center.y);
        const int brightnessLeft = pixelGrid.getAtPosition(sensors.left.x, sensors.left.y);
        const int brightnessRight = pixelGrid.getAtPosition(sensors.right.x, sensors.right.y);

        const float headingAngle = atan2(direction.y, direction.x);
        const float newAngle = [&]() {
            if (brightnessLeft > brightnessCenter && brightnessLeft > brightnessRight)
                return headingAngle + SENSING_ANGLE;
            if (brightnessRight > brightnessCenter && brightnessRight > brightnessLeft)
                return headingAngle - SENSING_ANGLE;
            return headingAngle;
        }();

        direction = float2 {cos(newAngle), sin(newAngle)} * travelSpeed;
    }

    void show()
    {
        const float headingAngle = atan2(direction.y, direction.x);
        const float angleLeft = headingAngle + SENSING_ANGLE;
        const float angleRight = headingAngle - SENSING_ANGLE;
        const float sensingIndicatorRadius = radius;

        const Sensors sensors = getSensors(SENSING_DISTANCE, SENSING_ANGLE);

        strokeWeight(1.0f);
        fill(255, 0, 0);
        noStroke();
        // circle(sensors.center.x, sensors.center.y, sensingIndicatorRadius * 2.0f);
        circle(sensors.left.x, sensors.left.y, sensingIndicatorRadius * 2.0f);

        fill(255, 0, 0);
        noStroke();
        circle(sensors.right.x, sensors.right.y, sensingIndicatorRadius * 2.0f);

        // stroke(255);
        // strokeWeight(1.0f);
        // line(position.x, position.y, sensors.center.x, sensors.center.y);
        // line(position.x, position.y, sensors.left.x, sensors.left.y);
        // line(position.x, position.y, sensors.right.x, sensors.right.y);

        // fill(100);
        // stroke(255);
        // strokeWeight(2);
        // circle(position.x, position.y, radius * 2.0f);
    }

    Sensors getSensors(float sensingDistance, float sensingAngle) const
    {
        const float headingAngle = atan2(direction.y, direction.x);
        const float angleLeft = headingAngle + sensingAngle;
        const float angleRight = headingAngle - sensingAngle;

        return Sensors {
            .center = {position.x + cos(headingAngle) * sensingDistance, position.y + sin(headingAngle) * sensingDistance},
            .left = {position.x + cos(angleLeft) * sensingDistance, position.y + sin(angleLeft) * sensingDistance},
            .right = {position.x + cos(angleRight) * sensingDistance, position.y + sin(angleRight) * sensingDistance},
        };
    }
};

struct SlimeMoldsSimulation : Sketch
{
    std::unique_ptr<PixelGrid> grid;
    std::vector<Mold> molds;

    void setup() override
    {
        setWindowSize(800, 800);
        setWindowResizable(false);
        setWindowTitle("Slime Molds Simulation");

        const size_t gridCellSize = 1;
        const size_t gridColumns = getWidth() / gridCellSize;
        const size_t gridRows = getHeight() / gridCellSize;

        grid = std::make_unique<PixelGrid>(gridColumns, gridRows);

        // for (size_t i = 0; i < 100000; ++i) {
        for (size_t i = 0; i < 100; ++i) {
            const float px = static_cast<float>(getWidth()) * 0.5f + randomFloat(-50, 50);
            const float py = static_cast<float>(getHeight()) * 0.5f + randomFloat(-50, 50);
            const float angle = randomFloat(TWO_PI);

            molds.push_back(Mold {
                .position = {px, py},
                .direction = float2 {cos(angle), sin(angle)}.fixedLength(1.0f),
                .travelSpeed = 80.0f,
                .radius = 0.5f,
            });
        }
    }

    void event(const WindowEvent& e) override
    {
        if (e.type == EventType::mousePress) {
            for (size_t i = 0; i < 10000; ++i) {
                molds.push_back(Mold {
                    .position = {static_cast<float>(e.mouseButton.x), static_cast<float>(e.mouseButton.y)},
                    .direction = float2 {cos(randomFloat(TWO_PI)), sin(randomFloat(TWO_PI))}.fixedLength(1.0f),
                    .travelSpeed = 80.0f,
                    .radius = 0.5f,
                });
            }
        }
    }

    void draw() override
    {
        background(21);

        grid->update(getDeltaTime());
        grid->show();

        for (Mold& mold : molds) {
            mold.update(*grid, getDeltaTime());
            // mold.show();
        }
    }
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<SlimeMoldsSimulation>();
    }
} // namespace p5cpp
