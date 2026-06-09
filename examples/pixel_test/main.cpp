#include <cfloat>
#include <p5.hpp>

using namespace p5;

struct PixelTestSketch : Sketch
{
    std::vector<TextOutlinePoint> points;

    void setup() override
    {
        points = getCurrentFont()->getTextOutline("Test", 28, 4);

        float minY = FLT_MAX, maxY = -FLT_MAX;
        for (auto& p : points) {
            minY = std::min(minY, p.position.y);
            maxY = std::max(maxY, p.position.y);
        }

        std::printf("minY: %f, maxY: %f\n", minY, maxY);
        std::fflush(stdout);
    }

    void event(const WindowEvent& e) override
    {
    }

    void draw() override
    {
        background(0);

        translate(100.0f, 220.0f);
        for (const TextOutlinePoint& point : points) {
            stroke(255);
            strokeWeight(2.0f);
            ::point(point.position.x, point.position.y);
        }

        translate(0.0f, 50.0f);
        textSize(28.0f);
        text("Test", 0, 0);
    }
};

namespace p5
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<PixelTestSketch>();
    }
} // namespace p5
