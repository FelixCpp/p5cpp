// A visual gallery exercising every rendering path touched by the OpenGL -> WebGPU migration,
// laid out as labeled panels so a human can scan the window and spot anything wrong at a glance.
// Not a replacement for the other example sketches (each of those is still the more natural,
// realistic exercise of its own feature) -- this exists purely as a single place to look.

#include <p5cpp/p5cpp.hpp>

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

using namespace p5;

namespace
{
    constexpr int kColumns = 4;
    constexpr int kRows = 3;
    constexpr float kMargin = 20.0f;
    constexpr float kGap = 16.0f;
    constexpr float kHeaderHeight = 56.0f;
    constexpr float kLabelHeight = 20.0f;

    rect2f cellRect(int column, int row)
    {
        const float gridWidth = getWidth() - 2.0f * kMargin;
        const float gridHeight = getHeight() - kHeaderHeight - kMargin - kLabelHeight * static_cast<float>(kRows);
        const float cellWidth = (gridWidth - static_cast<float>(kColumns - 1) * kGap) / static_cast<float>(kColumns);
        const float cellHeight = (gridHeight - static_cast<float>(kRows - 1) * kGap) / static_cast<float>(kRows);

        return rect2f {
            .left = kMargin + static_cast<float>(column) * (cellWidth + kGap),
            .top = kHeaderHeight + static_cast<float>(row) * (cellHeight + kLabelHeight + kGap),
            .width = cellWidth,
            .height = cellHeight,
        };
    }

    // Every panel gets the same frame: a label above it, a background plate, and content clipped
    // to its bounds (so a panel's own test -- e.g. blend modes bleeding past their circles, or the
    // clip() panel itself -- can never visually spill into its neighbors). Doubles as its own
    // exercise of clip()/noClip() twelve times over.
    template <std::invocable Func>
    void panel(std::string_view label, const rect2f& r, Func&& drawContent)
    {
        noStroke();
        fill(rgba(230));
        textAlign(TextAlignment::bottomLeft);
        textSize(13.0f);
        text(label, r.left + 2.0f, r.top - 4.0f);

        fill(rgba(26, 26, 32));
        rect(r.left, r.top, r.width, r.height, BorderRadius::all(6.0f));

        clip(r.left, r.top, r.width, r.height);
        drawContent();
        noClip();
    }

    Texture makeCheckerTexture()
    {
        constexpr uint32_t size = 8;
        std::vector<uint8_t> pixels(size * size * 4);
        for (uint32_t y = 0; y < size; ++y) {
            for (uint32_t x = 0; x < size; ++x) {
                const bool light = ((x + y) % 2) == 0;
                const size_t i = (static_cast<size_t>(y) * size + x) * 4;
                pixels[i + 0] = light ? 235 : 60;
                pixels[i + 1] = light ? 235 : 120;
                pixels[i + 2] = light ? 235 : 200;
                pixels[i + 3] = 255;
            }
        }
        return loadTexture(size, size, pixels).value();
    }
} // namespace

struct TestGallery : Sketch
{
    Texture photo;
    Texture invertedPhoto;
    Texture checker;
    std::vector<Texture> photoTiles;

    Shader waveShader;

    Graphics smoothTarget;
    Graphics sharpTarget;
    Graphics compositeLayer;
    Graphics pixelReaderSource;

    std::unique_ptr<PixelReader> pixelReader;
    float lastReadbackBrightness = 0.0f;
    uint32_t readbackFrameCount = 0;

    void setup() override
    {
        setWindowSize(1400, 1000);
        setWindowResizable(true);

        photo = loadTexture("assets/landscape.jpg").value();
        checker = makeCheckerTexture();

        // getSubTexture(): chop the photo into a shuffled grid of tiles.
        constexpr uint32_t tileColumns = 6, tileRows = 4;
        const float tileWidth = static_cast<float>(photo.size.x) / tileColumns;
        const float tileHeight = static_cast<float>(photo.size.y) / tileRows;
        for (uint32_t row = 0; row < tileRows; ++row) {
            for (uint32_t col = 0; col < tileColumns; ++col) {
                photoTiles.push_back(photo.getSubTexture(
                    static_cast<uint32_t>(col * tileWidth), static_cast<uint32_t>(row * tileHeight),
                    static_cast<uint32_t>(tileWidth), static_cast<uint32_t>(tileHeight)
                ));
            }
        }
        for (size_t i = photoTiles.size(); i > 1; --i) {
            const size_t j = static_cast<size_t>(random(0.0f, static_cast<float>(i - 1)));
            std::swap(photoTiles[i - 1], photoTiles[j]);
        }

        // loadPixels() / updatePixels() roundtrip: invert a copy of the photo on the CPU side.
        invertedPhoto = loadTexture("assets/landscape.jpg").value();
        Pixels pixels = invertedPhoto.loadPixels();
        for (size_t i = 0; i + 3 < pixels.data.size(); i += 4) {
            pixels.data[i + 0] = static_cast<uint8_t>(255 - pixels.data[i + 0]);
            pixels.data[i + 1] = static_cast<uint8_t>(255 - pixels.data[i + 1]);
            pixels.data[i + 2] = static_cast<uint8_t>(255 - pixels.data[i + 2]);
        }
        invertedPhoto.updatePixels(pixels);

        // Custom effect shader (WGSL, @group(2) extra uniforms via setUniform()).
        constexpr std::string_view waveShaderSource = R"(
            struct Uniforms {
                u_Time: f32,
                u_Strength: f32,
            };
            @group(2) @binding(0) var<uniform> u_Extra: Uniforms;

            fn effect(color: vec4f, image: texture_2d<f32>, imageSampler: sampler, texCoord: vec2f, screenCoord: vec2f) -> vec4f {
                let wave = sin(texCoord.y * 24.0 + u_Extra.u_Time * 2.5) * u_Extra.u_Strength;
                return textureSample(image, imageSampler, texCoord + vec2f(wave, 0.0)) * color;
            }
        )";
        waveShader = loadShaderFromMemory(waveShaderSource).value();

        // MSAA comparison targets: same content, different sample counts.
        smoothTarget = createGraphics(220, 220, 4).value();
        sharpTarget = createGraphics(220, 220, 0).value();

        // Offscreen composite layer, transformed (rotate/scale) when drawn back onto the main canvas.
        compositeLayer = createGraphics(180, 180).value();

        // Async PixelReader, exercised directly (not via p5cpp_gif) against a small animated target.
        pixelReaderSource = createGraphics(64, 64).value();
        pixelReader = createPixelReader(64, 64);
    }

    void drawShapesPanel(const rect2f& r)
    {
        strokeWeight(3.0f);
        stroke(rgba(255, 220, 80));
        fill(rgba(220, 90, 90));
        circle(r.left + r.width * 0.22f, r.top + r.height * 0.6f, 60.0f);

        noStroke();
        fill(rgba(90, 180, 220));
        rect(r.left + r.width * 0.42f, r.top + r.height * 0.3f, r.width * 0.24f, r.height * 0.35f, BorderRadius::all(10.0f));

        fill(rgba(120, 220, 140));
        triangle(
            r.left + r.width * 0.72f, r.top + r.height * 0.82f,
            r.left + r.width * 0.82f, r.top + r.height * 0.3f,
            r.left + r.width * 0.94f, r.top + r.height * 0.82f
        );

        noFill();
        stroke(rgba(255));
        strokeWeight(2.0f);
        ellipse(r.left + r.width * 0.5f, r.top + r.height * 0.18f, r.width * 0.15f, r.height * 0.08f);
    }

    void drawBlendModesPanel(const rect2f& r)
    {
        struct Entry
        {
            const char* label;
            BlendMode mode;
        };
        const Entry entries[] = {
            {"alpha", BlendMode::alpha},
            {"add", BlendMode::additive},
            {"mul", BlendMode::multiply},
            {"screen", BlendMode::screen},
        };

        const float cellWidth = r.width / 4.0f;
        for (int i = 0; i < 4; ++i) {
            const float cx = r.left + cellWidth * static_cast<float>(i) + cellWidth * 0.5f;
            const float cy = r.top + r.height * 0.42f;

            noStroke();
            blendMode(BlendMode::alpha);
            fill(rgba(230, 80, 80));
            circle(cx - 16.0f, cy, 42.0f);

            blendMode(entries[i].mode);
            fill(rgba(80, 140, 230, 210));
            circle(cx + 16.0f, cy, 42.0f);

            blendMode(BlendMode::alpha);
            fill(rgba(255));
            textAlign(TextAlignment::center);
            textSize(12.0f);
            text(entries[i].label, cx, r.top + r.height - 16.0f);
        }
    }

    void drawTextPanel(const rect2f& r)
    {
        fill(rgba(255));
        textAlign(TextAlignment::topLeft);
        textSize(30.0f);
        text("Aa Bb 123", r.left + 12.0f, r.top + 8.0f);

        textSize(15.0f);
        fill(rgba(190, 215, 255));
        text("glyph atlas (r8 texture)", r.left + 12.0f, r.top + 52.0f);

        textSize(12.0f);
        fill(rgba(160));
        text("The quick brown fox jumps over the lazy dog, rendered through the text fragment shader.", r.left + 12.0f, r.top + 78.0f, r.width - 24.0f);
    }

    void drawImagePanel(const rect2f& r)
    {
        image(photo, r.left, r.top, r.width, r.height);
    }

    void drawSubTexturePanel(const rect2f& r)
    {
        constexpr uint32_t tileColumns = 6, tileRows = 4;
        const float tileWidth = r.width / tileColumns;
        const float tileHeight = r.height / tileRows;
        for (uint32_t row = 0; row < tileRows; ++row) {
            for (uint32_t col = 0; col < tileColumns; ++col) {
                image(photoTiles[row * tileColumns + col], r.left + col * tileWidth, r.top + row * tileHeight, tileWidth, tileHeight);
            }
        }
    }

    void drawFilterWrapPanel(const rect2f& r)
    {
        const float halfWidth = r.width * 0.5f;
        const float topHeight = r.height * 0.55f;

        textureFilter(TextureFilter::nearest);
        textureWrap(TextureWrap::clampToEdge);
        image(checker, r.left + 4.0f, r.top + 4.0f, halfWidth - 8.0f, topHeight - 8.0f);

        textureFilter(TextureFilter::linear);
        image(checker, r.left + halfWidth + 4.0f, r.top + 4.0f, halfWidth - 8.0f, topHeight - 8.0f);

        fill(rgba(255));
        textAlign(TextAlignment::center);
        textSize(11.0f);
        text("nearest", r.left + halfWidth * 0.5f, r.top + topHeight - 2.0f);
        text("linear", r.left + halfWidth * 1.5f, r.top + topHeight - 2.0f);

        const float thirdWidth = (r.width - 8.0f) / 3.0f;
        const float bottomTop = r.top + topHeight + 8.0f;
        const float bottomHeight = r.height - topHeight - 12.0f;

        textureWrap(TextureWrap::repeat);
        image(checker, r.left, bottomTop, thirdWidth, bottomHeight, 0.0f, 0.0f, 3.0f, 3.0f);
        textureWrap(TextureWrap::mirroredRepeat);
        image(checker, r.left + thirdWidth + 4.0f, bottomTop, thirdWidth, bottomHeight, 0.0f, 0.0f, 3.0f, 3.0f);
        textureWrap(TextureWrap::clampToEdge);
        image(checker, r.left + (thirdWidth + 4.0f) * 2.0f, bottomTop, thirdWidth, bottomHeight, 0.0f, 0.0f, 3.0f, 3.0f);

        textureWrap(TextureWrap::clampToEdge);
    }

    void drawCustomShaderPanel(const rect2f& r)
    {
        withShader(waveShader, [&] {
            setUniform("u_Time", static_cast<float>(getGlobalTime()));
            setUniform("u_Strength", 0.03f);
            image(photo, r.left, r.top, r.width, r.height);
        });

        fill(rgba(255));
        textAlign(TextAlignment::bottomLeft);
        textSize(11.0f);
        text("custom WGSL effect() + setUniform()", r.left + 8.0f, r.top + r.height - 8.0f);
    }

    void drawMSAAPanel(const rect2f& r)
    {
        const float angle = static_cast<float>(getGlobalTime());
        const auto drawSpinner = [angle](const Graphics& target) {
            background(rgba(20, 20, 25));
            push();
            translate(static_cast<float>(target.size.x) * 0.5f, static_cast<float>(target.size.y) * 0.5f);
            rotate(angle);
            noStroke();
            fill(rgba(255, 200, 80));
            triangle(-40.0f, 30.0f, 0.0f, -50.0f, 40.0f, 30.0f);
            pop();
        };

        withGraphics(smoothTarget, [&] { drawSpinner(smoothTarget); }, false);
        withGraphics(sharpTarget, [&] { drawSpinner(sharpTarget); }, false);

        const float halfWidth = r.width * 0.5f;
        image(smoothTarget, r.left + 4.0f, r.top + 4.0f, halfWidth - 8.0f, r.height - 24.0f);
        image(sharpTarget, r.left + halfWidth + 4.0f, r.top + 4.0f, halfWidth - 8.0f, r.height - 24.0f);

        fill(rgba(255));
        textAlign(TextAlignment::center);
        textSize(11.0f);
        text("smooth (4x MSAA)", r.left + halfWidth * 0.5f, r.top + r.height - 12.0f);
        text("noSmooth", r.left + halfWidth * 1.5f, r.top + r.height - 12.0f);
    }

    void drawClipPanel(const rect2f& r)
    {
        const float clipWidth = r.width * 0.6f;
        const float clipHeight = r.height * 0.6f;
        const float clipLeft = r.left + (r.width - clipWidth) * 0.5f;
        const float clipTop = r.top + (r.height - clipHeight) * 0.5f;

        noFill();
        stroke(rgba(255, 255, 255, 90));
        strokeWeight(1.0f);
        rect(clipLeft, clipTop, clipWidth, clipHeight);

        clip(clipLeft, clipTop, clipWidth, clipHeight);
        noStroke();
        const float t = static_cast<float>(getGlobalTime());
        for (int i = 0; i < 6; ++i) {
            fill(rgba(80 + i * 28, 120, 220 - i * 20));
            circle(
                clipLeft + (clipWidth / 5.0f) * static_cast<float>(i),
                clipTop + clipHeight * 0.5f + std::sin(t + static_cast<float>(i)) * clipHeight * 0.3f,
                34.0f
            );
        }
        noClip();
    }

    void drawCompositePanel(const rect2f& r)
    {
        const float t = static_cast<float>(getGlobalTime());

        withGraphics(
            compositeLayer, [&] {
                background(rgba(26, 26, 32));
                noStroke();
                for (int i = 0; i < 5; ++i) {
                    const float a = t * 1.3f + static_cast<float>(i) * 1.1f;
                    fill(rgba(90 + i * 30, 200 - i * 20, 240));
                    circle(90.0f + std::cos(a) * 55.0f, 90.0f + std::sin(a) * 55.0f, 22.0f);
                }
            },
            false
        );

        push();
        translate(r.left + r.width * 0.5f, r.top + r.height * 0.5f);
        rotate(std::sin(t * 0.5f) * 0.3f);
        const float s = 0.85f + 0.1f * std::sin(t * 2.0f);
        scale(s, s);
        image(
            compositeLayer,
            -static_cast<float>(compositeLayer.size.x) * 0.5f, -static_cast<float>(compositeLayer.size.y) * 0.5f,
            static_cast<float>(compositeLayer.size.x), static_cast<float>(compositeLayer.size.y)
        );
        pop();
    }

    void drawPixelsRoundtripPanel(const rect2f& r)
    {
        image(invertedPhoto, r.left, r.top, r.width, r.height);

        fill(rgba(255));
        textAlign(TextAlignment::bottomLeft);
        textSize(11.0f);
        text("loadPixels() -> invert on CPU -> updatePixels()", r.left + 8.0f, r.top + r.height - 8.0f);
    }

    void drawPixelReaderPanel(const rect2f& r)
    {
        const float t = static_cast<float>(getGlobalTime());
        withGraphics(
            pixelReaderSource, [&] {
                background(rgba(10, 10, 15));
                noStroke();
                fill(rgba(
                    static_cast<int32_t>(128.0f + 127.0f * std::sin(t)),
                    static_cast<int32_t>(128.0f + 127.0f * std::sin(t + 2.1f)),
                    static_cast<int32_t>(128.0f + 127.0f * std::sin(t + 4.2f))
                ));
                circle(32.0f + std::cos(t * 2.0f) * 14.0f, 32.0f + std::sin(t * 2.0f) * 14.0f, 30.0f);
            },
            false
        );

        requestPixelReadback(*pixelReader, pixelReaderSource.colorTexture);
        if (std::optional<Pixels> pixels = pollPixelReadback(*pixelReader)) {
            uint64_t sum = 0;
            for (size_t i = 0; i + 2 < pixels->data.size(); i += 4) {
                sum += pixels->data[i] + pixels->data[i + 1] + pixels->data[i + 2];
            }
            const size_t pixelCount = pixels->data.size() / 4;
            lastReadbackBrightness = pixelCount > 0 ? static_cast<float>(sum) / static_cast<float>(pixelCount * 3) : 0.0f;
            ++readbackFrameCount;
        }

        // Stacked vertically (preview above, text below) rather than side by side -- robust down
        // to a narrow panel width, unlike a fixed left/right split.
        const float previewSize = std::min(r.height * 0.5f, r.width - 16.0f);
        image(pixelReaderSource, r.left + (r.width - previewSize) * 0.5f, r.top + 8.0f, previewSize, previewSize);

        fill(rgba(255));
        textAlign(TextAlignment::topLeft);
        textSize(11.0f);
        const float textTop = r.top + previewSize + 16.0f;
        text("createPixelReader() async readback", r.left + 8.0f, textTop, r.width - 16.0f);
        text("avg brightness: " + std::to_string(static_cast<int>(lastReadbackBrightness)), r.left + 8.0f, textTop + 32.0f, r.width - 16.0f);
        text("frames captured: " + std::to_string(readbackFrameCount), r.left + 8.0f, textTop + 50.0f, r.width - 16.0f);
    }

    void draw() override
    {
        background(rgba(15, 15, 18));

        fill(rgba(255));
        textAlign(TextAlignment::topLeft);
        textSize(22.0f);
        text("p5cpp WebGPU Test Gallery", kMargin, 10.0f);
        textSize(12.0f);
        fill(rgba(140));
        text("Every panel below exercises a distinct part of the OpenGL -> WebGPU migration", kMargin, 36.0f);

        auto withCell = [](int column, int row, auto&& drawFn) {
            const rect2f r = cellRect(column, row);
            drawFn(r);
        };

        withCell(0, 0, [&](const rect2f& r) { panel("Shapes & Stroke", r, [&] { drawShapesPanel(r); }); });
        withCell(1, 0, [&](const rect2f& r) { panel("Blend Modes", r, [&] { drawBlendModesPanel(r); }); });
        withCell(2, 0, [&](const rect2f& r) { panel("Text (glyph atlas)", r, [&] { drawTextPanel(r); }); });
        withCell(3, 0, [&](const rect2f& r) { panel("Image Texture", r, [&] { drawImagePanel(r); }); });

        withCell(0, 1, [&](const rect2f& r) { panel("getSubTexture() Tiles", r, [&] { drawSubTexturePanel(r); }); });
        withCell(1, 1, [&](const rect2f& r) { panel("Filter & Wrap Modes", r, [&] { drawFilterWrapPanel(r); }); });
        withCell(2, 1, [&](const rect2f& r) { panel("Custom Shader + Uniforms", r, [&] { drawCustomShaderPanel(r); }); });
        withCell(3, 1, [&](const rect2f& r) { panel("MSAA (smooth vs noSmooth)", r, [&] { drawMSAAPanel(r); }); });

        withCell(0, 2, [&](const rect2f& r) { panel("clip() / noClip()", r, [&] { drawClipPanel(r); }); });
        withCell(1, 2, [&](const rect2f& r) { panel("Offscreen Graphics Composite", r, [&] { drawCompositePanel(r); }); });
        withCell(2, 2, [&](const rect2f& r) { panel("loadPixels/updatePixels", r, [&] { drawPixelsRoundtripPanel(r); }); });
        withCell(3, 2, [&](const rect2f& r) { panel("Async PixelReader", r, [&] { drawPixelReaderPanel(r); }); });
    }
};

SketchSpec p5::createSpec()
{
    return {
        .sketch = [] {
            return std::make_unique<TestGallery>();
        }
    };
}
