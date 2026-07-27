#include <p5cpp/p5cpp.hpp>

#include <cmath>

// Demonstrates drawing sub-regions of a texture via the 9-argument image()
// overload (image(texture, dx, dy, dWidth, dHeight, sx, sy, sWidth, sHeight)),
// the building block for sprite sheet / character animation. Since we don't
// ship a sprite sheet asset, one is generated procedurally on an offscreen
// canvas in setup() — swap buildSpriteSheet() for loadImage("sheet.png") to
// use a real asset instead.
namespace
{
    using namespace p5cpp;

    constexpr int kColumns = 6;
    constexpr int kRows = 2;
    constexpr float kFrameSize = 48.0f;
    constexpr float kFramesPerSecond = 10.0f;

    struct SpriteAnimationSketch : Sketch
    {
        Framebuffer spriteSheet;

        void setup() override
        {
            setWindowSize(720, 360);
            setWindowTitle("Sprite Sheet Animation demo");
            frameRate(60);

            spriteSheet = createFramebuffer(static_cast<uint32_t>(kFrameSize) * kColumns, static_cast<uint32_t>(kFrameSize) * kRows);
            buildSpriteSheet();
        }

        // Draws a kColumns x kRows grid of walk-cycle frames into spriteSheet: row 0
        // is a blue character, row 1 an orange one, each with kColumns poses of a
        // swinging-leg walk cycle.
        void buildSpriteSheet()
        {
            pushCanvas(spriteSheet);
            background(0, 0);
            noStroke();

            for (int row = 0; row < kRows; ++row) {
                const color_t bodyColor = row == 0 ? rgba(90, 170, 250) : rgba(250, 150, 90);

                for (int col = 0; col < kColumns; ++col) {
                    const float phase = static_cast<float>(col) / static_cast<float>(kColumns) * TWO_PI;
                    const float legSwing = std::sin(phase) * 12.0f;
                    const float bob = std::abs(std::cos(phase)) * 3.0f;

                    push();
                    translate(col * kFrameSize + kFrameSize * 0.5f, row * kFrameSize + kFrameSize * 0.5f - bob);

                    fill(bodyColor);
                    rect(-9, -4, 18, 22, BorderRadius::circular(4));
                    circle(0, -16, 12);

                    push();
                    translate(-4, 16);
                    rotate(radians(legSwing));
                    rect(-3, 0, 6, 16, BorderRadius::circular(2));
                    pop();

                    push();
                    translate(4, 16);
                    rotate(radians(-legSwing));
                    rect(-3, 0, 6, 16, BorderRadius::circular(2));
                    pop();

                    pop();
                }
            }

            popCanvas();
        }

        void draw() override
        {
            background(30);

            const int totalFrames = kColumns;
            const int frame = (getFrameCount() * static_cast<int>(kFramesPerSecond) / 60) % totalFrames;

            fill(255);
            textSize(16.0f);
            textAlign(TextAlign::topLeft);
            text("textureMode(TextureMode::image): sx/sy/sWidth/sHeight in source-texture pixels", 20, 20);
            text("textureMode(TextureMode::normalized): sx/sy/sWidth/sHeight in 0..1 UV range", 20, 200);

            constexpr float displaySize = 144.0f;

            // Blue character, row 0 — pixel-space source rect.
            textureMode(TextureMode::image);
            image(*spriteSheet.getColorTexture(),
                  80, 50, displaySize, displaySize,
                  static_cast<float>(frame) * kFrameSize, 0.0f, kFrameSize, kFrameSize);

            // Orange character, row 1 — same animation, driven by a normalized source
            // rect instead, to show both textureMode() settings addressing the same
            // sprite sheet.
            textureMode(TextureMode::normalized);
            const float u = static_cast<float>(frame) / static_cast<float>(kColumns);
            image(*spriteSheet.getColorTexture(),
                  350, 230, displaySize, displaySize,
                  u, 0.5f, 1.0f / kColumns, 0.5f);
        }
    };
} // namespace

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<SpriteAnimationSketch>();
    }
} // namespace p5cpp
