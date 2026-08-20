#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>
using namespace p5;

struct InteractiveDisplay : Sketch
{
    float r = 15.0f;
    // float angle = 0.0f;
    float cSize = 10;
    size_t num = 15;
    float t = 10.0f;

    int cols = 10;
    int rows = 10;
    float size = 25;

    float inc = 1.7f;

    std::vector<float> angles;

    void setup() override
    {
        setWindowSize(400, 400);

        angles.resize(cols * rows, 0.0f);
        float xoff = 0.0f;
        for (size_t y = 0; y < rows; ++y) {
            float yoff = 0.0f;
            for (size_t x = 0; x < cols; ++x) {
                angles[y * cols + x] = noise(xoff, yoff) * 360.0f;
                yoff += inc;
            }
            xoff += inc;
        }
    }

    void draw() override
    {
        background(rgba(0, 0, 100));
        translate(getWidth() / 2, getHeight() / 2);
        noStroke();

        for (size_t y = 0; y < rows; ++y) {
            for (size_t x = 0; x < cols; ++x) {
                for (size_t i = 0; i < num; ++i) {
                    fill(rgba(255, 255 * (i / ((float)num - 1))));

                    const float angle = angles[y * cols + x];
                    const float offsetX = x * size - size * cols / 2 + size * 0.5f;
                    const float offsetY = y * size - size * rows / 2 + size * 0.5f;

                    float px = r * ((float)i / (num - 1)) * cos(radians(angle + t * i));
                    float py = r * ((float)i / (num - 1)) * sin(radians(angle + t * i));
                    ellipse(
                        offsetX + px + (x - float(cols / 2.0f)) * i * 0.7f,
                        offsetY + py + (y - float(rows / 2.0f)) * i * 0.7f,
                        (cSize + i) * 0.5f,
                        (cSize + i) * 0.5f
                    );
                }

                angles[y * cols + x] += 4.0f;
            }
        }
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<InteractiveDisplay>();
}
