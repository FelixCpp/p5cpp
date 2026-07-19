#include <p5cpp/p5cpp.hpp>
using namespace p5cpp;

struct EcosystemSketch : Sketch
{
    void setup() override
    {
        setWindowSize(800, 600);
    }

    void draw() override
    {
        background(21);

        float hexagonRadius = 50.0f;
        for (size_t x = 0; x < 10; ++x) {
            for (size_t y = 0; y < 10; ++y) {
                const float offsetY = x % 2 == 0 ? 0.0f : (hexagonRadius * sqrt(3.0f) * 0.5f);
                const float px = 100.0f + x * hexagonRadius * 1.5f;
                const float py = 100.0f - offsetY + y * hexagonRadius * sqrt(3.0f);
                drawHexagon(px, py, hexagonRadius);
            }
        }
    }

    void drawHexagon(float x, float y, float radius)
    {
        stroke(0);
        strokeWeight(2);
        beginShape();
        for (int i = 0; i < 6; ++i) {
            const float angle = TWO_PI / 6 * i;
            const float px = x + radius * cos(angle);
            const float py = y + radius * sin(angle);
            vertex(px, py);
        }
        endShape(ShapeType::polygon, true);
    }
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<EcosystemSketch>();
    }
} // namespace p5cpp
