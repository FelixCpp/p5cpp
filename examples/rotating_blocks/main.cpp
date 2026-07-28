#include <cmath>
#include <p5cpp/p5cpp.hpp>

namespace
{
    using namespace p5cpp;

    struct Block
    {
        float2 position;
        float2 size;
        float rotation;
        bool isAffectedByMouse;

        explicit Block(const float2& position, const float2& size)
            : position(position), size(size), rotation(0.0f), isAffectedByMouse(false)
        {
        }

        void lookAt(const int2 point)
        {
            static constexpr float affectionRadius = 50.0f;

            const float2 direction = float2 {point} - position;
            isAffectedByMouse = lengthSquared(direction) < (affectionRadius * affectionRadius);

            if (isAffectedByMouse) {
                const float angle = std::atan2(direction.y, direction.x);
                rotation = angle;
            } else {
                // Slowly return to the original rotation (0.0f) when the mouse is not affecting the block
                rotation *= 0.95f; // Adjust the factor for desired speed of return
            }
        }

        void update()
        {
        }

        void show()
        {
            const bool isBlock = isAffectedByMouse or (std::abs(rotation) > radians(5.0f));

            pushState();
            pushMatrix();
            translate(position.x, position.y);
            rotate(rotation);

            const float width = size.x * 0.5f;
            const float height = size.y * 0.5f;

            const float left = -width;
            const float top = -height;
            const float right = width;
            const float bottom = height;

            float alpha = std::clamp(remap(std::abs(rotation), 0.0f, radians(90.0f), 0.0f, 255.0f), 100.0f, 255.0f);
            if (isAffectedByMouse) {
                alpha = 255.0f;
            }

            const color_t color = hsv(degrees(rotation), 0.8f, 1.0f, alpha);
            const float border = remap(std::abs(rotation), 0.0f, radians(90.0f), 1.0f, 4.0f);

            if (isBlock) {
                noFill();
                stroke(color);
                strokeWeight(border);
                rect(left, top, size.x, size.y);
            } else {

                stroke(color);
                strokeWeight(border);
                line(left, top, right, bottom);
                line(right, top, left, bottom);
            }
            popMatrix();
            popState();
        }
    };

    constexpr const char* pixelateSource = R"(
        uniform float u_BlockSize;

        vec4 effect(vec2 uv, vec2 texelSize, sampler2D tex) {
            vec2 block = texelSize * max(u_BlockSize, 1.0);
            vec2 blockUV = (floor(uv / block) + 0.5) * block;
            return texture(tex, blockUV);
        }
    )";

    struct RotatingBlocksSketch : Sketch
    {
        inline static constexpr int W = 900;
        inline static constexpr int H = 900;

        inline static constexpr int COLS = 40;
        inline static constexpr int ROWS = 40;

        inline static constexpr int BLOCK_WIDTH = W / COLS;
        inline static constexpr int BLOCK_HEIGHT = H / ROWS;
        Shader pixelateShader;
        Framebuffer scene;

        std::vector<Block> blocks;

        void setup() override
        {
            setWindowSize(W, H);
            setWindowTitle("Interactive Display with rotating Blocks");

            scene = createFramebuffer(W, H);
            pixelateShader = loadEffectShader(pixelateSource);

            constexpr float padding = static_cast<float>(BLOCK_WIDTH) * 0.5f;
            constexpr float2 blockSize {static_cast<float>(BLOCK_WIDTH) - padding, static_cast<float>(BLOCK_HEIGHT) - padding};

            for (int y = 0; y <= ROWS; ++y) {
                for (int x = 0; x <= COLS; ++x) {
                    const float positionX = static_cast<float>((x + 0.5f) * BLOCK_WIDTH);
                    const float positionY = static_cast<float>((y + 0.5f) * BLOCK_HEIGHT);
                    blocks.emplace_back(float2 {positionX, positionY}, blockSize);
                }
            }
        }

        void draw() override
        {
            pushCanvas(scene);
            {
                background(0);
                const int2 mousePosition = int2 {getMouseX(), getMouseY()};

                for (Block& block : blocks) {
                    block.lookAt(mousePosition);
                    block.update();
                    block.show();
                }
            }
            popCanvas();

            shader(pixelateShader);
            setUniform(pixelateShader, "u_BlockSize", uniform(4.0f));
            setUniform(pixelateShader, "u_TexelSize", uniform(1.0f / static_cast<float>(W), 1.0f / static_cast<float>(H)));
            image(*scene.getColorTexture(), 0, 0, W, H);
            noShader();
        }
    };
} // namespace

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<RotatingBlocksSketch>();
    }
} // namespace p5cpp
