// render_group_demo.cpp
// -----------------------------------------------------------------------
// Demonstrates buildRenderGroup()/drawRenderGroup(): geometry is tessellated
// once and replayed cheaply every frame instead of being rebuilt from
// scratch on every call.
//
//   - blobGroup      a many-vertex irregular polygon (fill + stroke), drawn
//                     BLOB_COPIES times per frame at different transforms,
//                     to make the CPU-cost win directly observable (compare
//                     the FPS counter against the "immediate mode" toggle).
//   - texturedGroup   an image() call recorded inside buildRenderGroup().
//   - shaderGroup      a custom shader() + setUniform() recorded inside
//                     buildRenderGroup() - the uniform value is frozen at
//                     build time; changing it afterwards does not affect
//                     already-recorded groups, proving the snapshot is
//                     independent of the live uniform cache.
//   - composedGroup   built by calling drawRenderGroup() on blobGroup and
//                     shaderGroup *inside* another buildRenderGroup() call -
//                     composition of groups from groups.
//   - textGroup       a text() call recorded inside buildRenderGroup().
//
// Steuerung:
//   Space   - toggle immediate-mode comparison (rebuilds the blob polygon
//             from scratch every frame instead of replaying blobGroup)
//   Escape  - beenden
// -----------------------------------------------------------------------

#include <p5cpp/p5cpp.hpp>

#include <cmath>
#include <format>
#include <vector>

using namespace p5cpp;

namespace
{
    constexpr int BLOB_POINTS = 240;
    constexpr int BLOB_COPIES = 60;

    constexpr const char* pulseVSource = R"(
        #version 410 core

        layout (location = 0) in vec2 a_Position;
        layout (location = 1) in vec2 a_TexCoord;
        layout (location = 2) in vec4 a_Color;
        layout (location = 3) in float a_TexIndex;

        out vec2 v_TexCoord;
        out vec4 v_Color;
        out float v_TexIndex;

        uniform mat4 u_ProjectionMatrix;

        void main() {
            gl_Position = u_ProjectionMatrix * vec4(a_Position, 0.0, 1.0);
            v_TexCoord = a_TexCoord;
            v_Color = a_Color;
            v_TexIndex = a_TexIndex;
        }
    )";

    // Multiplies the baked vertex color by u_Pulse - frozen at buildRenderGroup()
    // time via setUniform(), independent of whatever u_Pulse is set to later.
    constexpr const char* pulseFSource = R"(
        #version 410 core

        layout (location = 0) out vec4 o_Color;

        in vec2 v_TexCoord;
        in vec4 v_Color;
        in float v_TexIndex;

        uniform sampler2D u_Textures[8];
        uniform float u_Pulse;

        void main() {
            vec4 texColor = vec4(1.0);
            switch (int(v_TexIndex)) {
                case 0: texColor = texture(u_Textures[0], v_TexCoord); break;
                case 1: texColor = texture(u_Textures[1], v_TexCoord); break;
                default: break;
            }
            o_Color = vec4(v_Color.rgb * u_Pulse, v_Color.a) * texColor;
        }
    )";

    // Builds the many-vertex irregular polygon geometry directly onto whatever
    // is currently active (the live canvas, or a buildRenderGroup() session) -
    // used both by the cached blobGroup and by the immediate-mode comparison path.
    void drawBlobPolygon(float time)
    {
        fill(230, 110, 60);
        stroke(20, 10, 10);
        strokeWeight(2.0f);

        beginShape();
        for (int i = 0; i < BLOB_POINTS; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(BLOB_POINTS);
            const float angle = t * TWO_PI;
            const float wobble = 1.0f + 0.15f * std::sin(angle * 7.0f + time) + 0.08f * std::sin(angle * 13.0f - time * 1.7f);
            const float radius = 18.0f * wobble;
            vertex(std::cos(angle) * radius, std::sin(angle) * radius);
        }
        endShape(ShapeType::polygon, true);
    }

    Texture createCheckerTexture()
    {
        constexpr uint32_t size = 8;
        std::vector<color_t> pixels(size * size);
        for (uint32_t y = 0; y < size; ++y) {
            for (uint32_t x = 0; x < size; ++x) {
                const bool light = ((x + y) % 2) == 0;
                pixels[y * size + x] = light ? rgba(255, 230, 120, 255) : rgba(60, 40, 120, 255);
            }
        }
        return loadTexture(size, size, pixels.data());
    }
} // namespace

class RenderGroupDemoSketch : public Sketch
{
public:
    void setup() override
    {
        setWindowSize(1000, 700);
        setWindowTitle("p5cpp - RenderGroup Demo");
        frameRate(0); // uncapped, so the FPS counter reflects the actual per-frame cost

        checkerTexture = createCheckerTexture();
        pulseShader = loadShader(pulseVSource, pulseFSource);

        blobGroup = buildRenderGroup([]() {
            drawBlobPolygon(0.0f);
        });

        texturedGroup = buildRenderGroup([this]() {
            noStroke();
            tint(255);
            image(checkerTexture, -40.0f, -40.0f, 80.0f, 80.0f);
        });

        shaderGroup = buildRenderGroup([this]() {
            shader(pulseShader);
            setUniform(pulseShader, "u_Pulse", uniform(1.0f)); // frozen: always full brightness on replay
            noStroke();
            fill(80, 180, 255);
            circle(0.0f, 0.0f, 70.0f);
            noShader();
        });

        // Composition: a group built purely out of drawRenderGroup() calls to
        // other, already-built groups.
        composedGroup = buildRenderGroup([this]() {
            pushMatrix();
            translate(-35.0f, 0.0f);
            drawRenderGroup(shaderGroup);
            popMatrix();

            pushMatrix();
            translate(35.0f, 0.0f);
            scale(1.5f, 1.5f);
            drawRenderGroup(blobGroup);
            popMatrix();
        });

        // Changing the live uniform after building shaderGroup must NOT affect
        // shaderGroup's replay - it already carries its own frozen snapshot.
        setUniform(pulseShader, "u_Pulse", uniform(0.15f));

        textGroup = buildRenderGroup([this]() {
            fill(255, 220, 140);
            textAlign(TextAlign::topLeft);
            textSize(20.0f);
            text("Cached via text()\ninside buildRenderGroup()", 0.0f, 0.0f);
        });
    }

    void event(const WindowEvent& e) override
    {
        if (e.type == EventType::keyPress && e.keyEvent.key == Key::k) {
            immediateMode = !immediateMode;
        }
    }

    void draw() override
    {
        background(18, 18, 24);

        const float time = getGlobalTime();
        const int cols = 10;
        const int rows = (BLOB_COPIES + cols - 1) / cols;
        const float cellW = 620.0f / static_cast<float>(cols);
        const float cellH = 420.0f / static_cast<float>(rows);

        for (int i = 0; i < BLOB_COPIES; ++i) {
            const int col = i % cols;
            const int row = i / cols;
            const float x = 40.0f + cellW * (static_cast<float>(col) + 0.5f);
            const float y = 40.0f + cellH * (static_cast<float>(row) + 0.5f);

            pushMatrix();
            translate(x, y);
            rotate(time * 0.6f + static_cast<float>(i) * 0.35f);

            if (immediateMode) {
                drawBlobPolygon(time + static_cast<float>(i));
            } else {
                drawRenderGroup(blobGroup);
            }

            popMatrix();
        }

        pushMatrix();
        translate(760.0f, 120.0f);
        drawRenderGroup(texturedGroup);
        popMatrix();

        pushMatrix();
        translate(760.0f, 260.0f);
        drawRenderGroup(shaderGroup);
        popMatrix();

        pushMatrix();
        translate(760.0f, 420.0f);
        drawRenderGroup(composedGroup);
        popMatrix();

        pushMatrix();
        translate(760.0f, 480.0f);
        drawRenderGroup(textGroup);
        popMatrix();

        fill(255);
        noStroke();
        textAlign(TextAlign::topLeft);
        textSize(16.0f);
        text(std::format("FPS: {}", getFrameRate()), 20.0f, 640.0f);
        text(std::format("Mode: {}  (Space to toggle)", immediateMode ? "immediate (rebuilds every frame)" : "cached RenderGroup"), 20.0f, 662.0f);
    }

private:
    RenderGroup blobGroup;
    RenderGroup texturedGroup;
    RenderGroup shaderGroup;
    RenderGroup composedGroup;
    RenderGroup textGroup;
    Texture checkerTexture;
    Shader pulseShader;
    bool immediateMode = false;
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<RenderGroupDemoSketch>();
    }
} // namespace p5cpp
