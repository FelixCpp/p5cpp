#include <p5cpp/p5cpp.hpp>

using namespace p5;

std::optional<Shader> createGaussianBlurShader()
{
    const std::string_view blurShaderCode = R"(
        struct Uniforms {
            u_BlurRadius: f32,
            u_Resolution: vec2f,
        };
        @group(2) @binding(0) var<uniform> u_Extra: Uniforms;

        fn effect(color: vec4f, image: texture_2d<f32>, imageSampler: sampler, texCoord: vec2f, screenCoord: vec2f) -> vec4f {
            let texelSize = 1.0 / u_Extra.u_Resolution;
            var outColor = vec4f(0.0);
            var totalWeight = 0.0;

            let kernelRadius = i32(ceil(u_Extra.u_BlurRadius));

            for (var ix = -kernelRadius; ix <= kernelRadius; ix++) {
                for (var iy = -kernelRadius; iy <= kernelRadius; iy++) {
                    let x = f32(ix);
                    let y = f32(iy);
                    let weight = exp(-(x * x + y * y) / (2.0 * u_Extra.u_BlurRadius * u_Extra.u_BlurRadius));
                    outColor += textureSample(image, imageSampler, texCoord + vec2f(x, y) * texelSize) * weight;
                    totalWeight += weight;
                }
            }

            return (outColor / totalWeight) * color;
        }
    )";

    return loadShaderFromMemory(blurShaderCode);
}

struct VPainting : Sketch
{
    Texture landscape = loadTexture("assets/landscape.jpg").value();
    Shader blurShader = createGaussianBlurShader().value();
    std::vector<Texture> shuffledTextures;

    float cellWidth;
    float cellHeight;
    uint32_t columns;
    uint32_t rows;

    void setup() override
    {
        const uint32_t landscapeWidth = landscape.size.x;
        const uint32_t landscapeHeight = landscape.size.y;
        const float aspectRatio = static_cast<float>(landscapeWidth) / landscapeHeight;
        const uint32_t windowWidth = 900;
        const uint32_t windowHeight = static_cast<uint32_t>(windowWidth / aspectRatio);

        setWindowSize(windowWidth, windowHeight);
        setWindowResizable(false);

        columns = 10;
        rows = 8;

        const float textureCellWidth = static_cast<float>(landscapeWidth) / columns;
        const float textureCellHeight = static_cast<float>(landscapeHeight) / rows;

        for (uint32_t row = 0; row < rows; ++row) {
            for (uint32_t col = 0; col < columns; ++col) {
                const uint32_t x = static_cast<uint32_t>(col * textureCellWidth);
                const uint32_t y = static_cast<uint32_t>(row * textureCellHeight);
                const uint32_t w = static_cast<uint32_t>(textureCellWidth);
                const uint32_t h = static_cast<uint32_t>(textureCellHeight);

                Texture cellTexture = landscape.getSubTexture(x, y, w, h);
                shuffledTextures.push_back(std::move(cellTexture));
            }
        }

        for (size_t row = 0; row < rows; ++row) {
            for (size_t col = 0; col < columns; ++col) {
                const size_t index = row * columns + col;
                const size_t minIndex = row * columns;
                const size_t maxIndex = row * columns + columns - 1;
                const size_t randomIndex = static_cast<size_t>(random(minIndex, maxIndex));
                std::swap(shuffledTextures[index], shuffledTextures[randomIndex]);
            }
        }
    }

    void draw() override
    {
        const float width = static_cast<float>(getWidth());
        const float height = static_cast<float>(getHeight());

        const float cellWidth = static_cast<float>(width) * 0.65f / static_cast<float>(columns);
        const float cellHeight = static_cast<float>(height) * 0.55f / static_cast<float>(rows);

        withShader(blurShader, [&]() {
            // setUniform("u_BlurRadius", 3.0f + (std::sin(getGlobalTime()) * 0.5f + 0.5f) * (10.0f - 3.0f));
            setUniform("u_BlurRadius", 10.0f);
            setUniform("u_Resolution", float2 {static_cast<float>(landscape.size.x), static_cast<float>(landscape.size.y)});
            image(landscape, 0, 0, width, height);
        });

        for (uint32_t row = 0; row < rows; ++row) {
            for (uint32_t column = 0; column < columns; ++column) {
                const size_t index = row * columns + column;
                const float offsetX = static_cast<float>(column) * cellWidth;
                const float offsetY = static_cast<float>(row) * cellHeight;
                const float startX = width * 0.5f - (columns * cellWidth) * 0.5f;
                const float startY = height * 0.5f - (rows * cellHeight) * 0.5f;
                const float positionX = startX + offsetX;
                const float positionY = startY + offsetY;

                Texture& cellTexture = shuffledTextures[index];
                image(cellTexture, positionX, positionY, cellWidth, cellHeight);
            }
        }
    }
};

SketchSpec p5::createSpec()
{
    return {
        .sketch = [] {
            return std::make_unique<VPainting>();
        }
    };
}
