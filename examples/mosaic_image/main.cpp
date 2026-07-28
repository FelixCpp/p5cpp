#include <p5cpp/p5cpp.hpp>
using namespace p5cpp;

struct MosaicImage : Sketch
{
    Texture frog;
    Pixels frogPixels;
    Framebuffer offscreenFrog;

    void setup() override
    {
        setWindowSize(800, 600);
        setWindowResizable(false);

        frog = loadImage("example_assets/beach.png");
        offscreenFrog = createFramebuffer(frog.getSize().x, frog.getSize().y);

        pushCanvas(offscreenFrog);
        {
            background(0, 0);
            image(frog, 0.0f, 0.0f, frog.getSize().x, frog.getSize().y);
            frogPixels = loadPixels();
        }
        popCanvas();
    }

    void draw() override
    {
        const auto [canvasWidth, canvasHeight] = getCanvasSize();

        background(30);

        const auto [frogWidth, frogHeight] = frog.getSize();

        static constexpr float dotSize = 6.0f;
        static constexpr float spacing = dotSize + 2.0f;
        const int columns = static_cast<int>(canvasWidth / spacing);
        const int rows = static_cast<int>(canvasHeight / spacing);
        pushMatrix();
        translate((canvasWidth - (columns - 1) * spacing) * 0.5f, (canvasHeight - (rows - 1) * spacing) * 0.5f);

        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < columns; ++col) {
                randomSeed(static_cast<unsigned int>(row * columns + col));
                const float randomOffsetX = randomFloat(-2.0f, 2.0f);
                const float randomOffsetY = 0.0f; // randomFloat(-spacing * 0.5f, spacing * 0.5f);
                const float scaleFactor = 1.0f;   // randomFloat(0.5f, 1.5f);

                const float x = col * spacing;
                const float y = row * spacing;

                const int frogX = static_cast<int>(x / canvasWidth * frogWidth);
                const int frogY = static_cast<int>(y / canvasHeight * frogHeight);

                const color_t pixelColor = frogPixels.get(frogX, frogY);
                fill(pixelColor);
                noStroke();
                ellipse(x + randomOffsetX, y + randomOffsetY, dotSize * 0.5f * scaleFactor, dotSize * 0.5f * scaleFactor);
            }
        }
        popMatrix();

        if (true) {
            textSize(64.0f);
            textWrap(TextWrap::word);

            constexpr std::string_view text = "Convert images to mosaics in p5cpp";
            const TextLayout layout = textLayout(text, 0.0f, 0.0f, canvasWidth * 0.75f);

            const float horizontalPadding = 24.0f;
            const float verticalPadding = 16.0f;
            const float boxWidth = layout.width + horizontalPadding * 2.0f;
            const float boxHeight = layout.height + verticalPadding * 2.0f;

            pushMatrix();
            translate(canvasWidth * 0.5f, canvasHeight * 0.5f);
            noStroke();
            fill(0, 220);
            rect(-boxWidth * 0.5f, -boxHeight * 0.5f, boxWidth, boxHeight, BorderRadius::circular(12.0f));
            fill(255);
            textAlign(TextAlign::center);
            ::text(text, 0.0f, 0.0f, canvasWidth * 0.75f);
            popMatrix();
        }

        // noLoop();
    }
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<MosaicImage>();
    }
} // namespace p5cpp
