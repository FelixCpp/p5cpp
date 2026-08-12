#include <p5cpp/p5cpp.hpp>

#include <cstdlib>
#include <ctime>

using namespace p5;



struct HelloWorldSketch : public Sketch
{
    std::shared_ptr<Framebuffer> offscreen = createFramebuffer(800, 600);
    std::vector<float2> positions;
    std::vector<color_t> colors;

    void setup() override
    {
        srand(time(nullptr));
        setWindowSize(800, 600);
    }

    void draw() override
    {
        withFramebuffer(offscreen, [&]() {
            background(rgba(51, 255));

            if (isMouseButtonPressed(MouseButton::Left)) {
                positions.push_back(float2 {(float)getMouseX(), (float)getMouseY()});

                int r = rand() % 255;
                int g = rand() % 255;
                int b = rand() % 255;

                colors.push_back(rgba(r, g, b));
            }

            fill(255);
            strokeWeight(15.0f);
            strokeJoin(StrokeJoin::round);
            strokeCap(StrokeCap::triangle);
            beginShape(ShapeMode::path);
            for (size_t i = 0; i < positions.size(); ++i) {
                stroke(colors[i]);
                vertex(positions[i].x, positions[i].y);
            }
            endShape(false);
        });

        background(rgba(255));
        image(offscreen->getColorTexture(), 0.0f, 0.0f, 800.0f, 600.0f);
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<HelloWorldSketch>();
}
