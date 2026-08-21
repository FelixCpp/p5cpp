#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>
using namespace p5;

struct DiscoGrid : Sketch
{
    std::vector<float> sizes;
    size_t columns;
    size_t rows;
    size_t cellSize;

    void setup() override
    {
        setWindowSize(400, 400);

        cellSize = 10;
        columns = getWindowSize().x / cellSize;
        rows = getWindowSize().y / cellSize;
    }

    void draw() override
    {
        background(rgba(220));
        noStroke();

        std::vector<float> sizes(columns * rows, 0.0f);
        constexpr float inc = 0.1f;
        float yoff = 0.0f;
        static float zoff = 0.0f;
        for (size_t y = 0; y < rows; ++y) {
            float xoff = 0.0f;
            for (size_t x = 0; x < columns; ++x) {
                const float noiseValue = noise(xoff, yoff, zoff);
                const float size = map(noiseValue, 0.0f, 1.0f, 0.0f, cellSize * 1.7f);
                sizes[y * columns + x] = size;

                const float r = noise(zoff) * 255.0f;
                const float g = noise(zoff + 15.0f) * 255.0f;
                const float b = noise(zoff + 30.0f) * 255.0f;

                fill(rgba(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b)));

                rect(
                    cellSize * 0.5f + x * cellSize - size * 0.5f,
                    cellSize * 0.5f + y * cellSize - size * 0.5f,
                    size,
                    size
                );

                xoff += inc;
            }
            yoff += inc;
            zoff += 0.00005f;
        }
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<DiscoGrid>();
}
