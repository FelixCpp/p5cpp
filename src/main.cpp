#include "p5.hpp"

#include <memory>

using namespace p5;

struct Starfield : p5::Sketch
{
public:
    std::shared_ptr<Font> font;
    std::shared_ptr<Canvas> redRect;
    std::shared_ptr<Canvas> blueRect;

    void setup() override
    {
        font = loadFont("Skia.ttf");
        std::fprintf(stdout, "Creating red\n");
        std::fflush(stdout);
        redRect = createCanvas(200, 200);
        std::fprintf(stdout, "Creating blue\n");
        std::fflush(stdout);
        blueRect = createCanvas(200, 200);

        pushCanvas(redRect);
        {
            info("Red Start");
            background(0);

            pushCanvas(blueRect);
            {
                info("Blue Start");
                background(0);
                fill(0, 0, 255);
                rect(50.0f, 50.0f, 100.0f, 100.0f);
                info("Blue End");
            }
            popCanvas();

            fill(255, 0, 0);
            rect(50.0f, 50.0f, 100.0f, 100.0f);
            info("Red End");
        }
        popCanvas();
    }

    void draw() override
    {
        background(51);

        std::fprintf(stdout, "blueRect textureId: %u\n", blueRect->getTextureId());
        std::fprintf(stdout, "redRect textureId: %u\n", redRect->getTextureId());

        image(blueRect->getTextureId(), 100.0f, 100.0f, 300.0f, 300.0f);
        image(redRect->getTextureId(), 410.0f, 100.0f, 300.0f, 300.0f);
    }

    void destroy() override
    {
    }

    void mousePressed(int x, int y) override
    {
    }
};

namespace p5
{
    std::unique_ptr<p5::Sketch> createSketch()
    {
        return std::make_unique<Starfield>();
    }
} // namespace p5
