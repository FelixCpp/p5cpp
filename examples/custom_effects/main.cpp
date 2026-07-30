// custom_effects.cpp
// -----------------------------------------------------------------------
// Demonstrates p5cpp's post-processing shaders. There is no dedicated
// "effect"/"filter" API - a post-processing shader is just a Shader, applied
// exactly like any other shader() call while drawing a Framebuffer's
// contents back out via image():
//
//   pushCanvas(scene);
//     ...draw the actual scene...
//   popCanvas();
//
//   shader(someShader);
//   setUniform(someShader, "u_Amount", uniform(1.0f));
//   image(scene.colorTexture, 0, 0, W, H);
//   noShader();
//
// loadGrayscaleShader()/loadInvertShader()/loadThresholdShader()/
// loadBlurShader() (see shader.hpp) are built-in convenience shaders written
// this exact way - nothing about them is special beyond being a starting
// point. loadEffectShader(): a lightweight way to write your OWN full-screen
// shader, in addition to the built-ins - only the pixel function needs to be
// written:
//
//     vec4 effect(vec2 uv, vec2 texelSize, sampler2D tex)
//
// loadEffectShader() wraps that snippet with the vertex shader and texture
// sampling boilerplate a full-screen pass needs, so callers never have to
// write either by hand.
//
// Steuerung:
//   1 - kein Effekt
//   2 - Blur       (built-in, loadBlurShader())
//   3 - Grayscale  (built-in, loadGrayscaleShader())
//   4 - Invert     (built-in, loadInvertShader())
//   5 - Threshold  (built-in, loadThresholdShader())
//   6 - Vignette   (custom effect via loadEffectShader)
//   7 - Pixelate   (custom effect via loadEffectShader)
//   Escape - beenden
// -----------------------------------------------------------------------

#include <p5cpp/p5cpp.hpp>

#include <cmath>
#include <string>

using namespace p5cpp;

namespace
{
    enum class EffectMode {
        none,
        blur,
        grayscale,
        invert,
        threshold,
        vignette,
        pixelate,
    };

    std::string effectName(EffectMode mode)
    {
        switch (mode) {
            case EffectMode::none: return "None";
            case EffectMode::blur: return "Blur (built-in)";
            case EffectMode::grayscale: return "Grayscale (built-in)";
            case EffectMode::invert: return "Invert (built-in)";
            case EffectMode::threshold: return "Threshold (built-in)";
            case EffectMode::vignette: return "Vignette (custom effect)";
            case EffectMode::pixelate: return "Pixelate (custom effect)";
        }
        return "?";
    }

    // Only the `effect()` function needs to be written - loadEffectShader()
    // supplies the surrounding vertex/fragment boilerplate. Custom effects can
    // declare their own uniforms too, set via setUniform() just like any
    // other shader.
    constexpr const char* vignetteSource = R"(
        uniform float u_Strength;

        vec4 effect(vec2 uv, vec2 texelSize, sampler2D tex) {
            vec4 c = texture(tex, uv);
            vec2 centered = uv - 0.5;
            float vignette = 1.0 - dot(centered, centered) * u_Strength;
            return vec4(c.rgb * clamp(vignette, 0.0, 1.0), c.a);
        }
    )";

    constexpr const char* pixelateSource = R"(
        uniform float u_BlockSize;

        vec4 effect(vec2 uv, vec2 texelSize, sampler2D tex) {
            vec2 block = texelSize * max(u_BlockSize, 1.0);
            vec2 blockUV = (floor(uv / block) + 0.5) * block;
            return texture(tex, blockUV);
        }
    )";
} // namespace

class CustomEffectsSketch : public Sketch
{
public:
    static constexpr int W = 960;
    static constexpr int H = 720;

    void setup() override
    {
        setWindowSize(W, H);
        setWindowTitle("p5cpp - Custom Effects");
        frameRate(60);

        // The actual scene is rendered into its own Framebuffer rather than straight onto
        // the window canvas, since a shader pass needs to read "everything drawn so far"
        // as a texture - and a texture can't be read and written by the same draw call.
        scene = createFramebuffer(W, H);
        blurScratch = createFramebuffer(W, H);

        blurShader = loadBlurShader();
        grayscaleShader = loadGrayscaleShader();
        invertShader = loadInvertShader();
        thresholdShader = loadThresholdShader();
        vignetteShader = loadEffectShader(vignetteSource);
        pixelateShader = loadEffectShader(pixelateSource);
    }

    void event(const WindowEvent& e) override
    {
        if (e.type != EventType::keyPress) return;

        switch (e.keyEvent.key) {
            case Key::n1: mode = EffectMode::none; break;
            case Key::n2: mode = EffectMode::blur; break;
            case Key::n3: mode = EffectMode::grayscale; break;
            case Key::n4: mode = EffectMode::invert; break;
            case Key::n5: mode = EffectMode::threshold; break;
            case Key::n6: mode = EffectMode::vignette; break;
            case Key::n7: mode = EffectMode::pixelate; break;
            case Key::escape: quit(); break;
            default: break;
        }
    }

    void draw() override
    {
        time += getDeltaTime();

        pushCanvas(scene);
        {
            background(18, 18, 24);

            noStroke();
            const int shapeCount = 24;
            for (int i = 0; i < shapeCount; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(shapeCount);
                const float angle = t * TWO_PI + time * 0.6f;
                const float radius = 180.0f + 60.0f * std::sin(time * 0.8f + t * TWO_PI * 3.0f);

                const float x = static_cast<float>(W) * 0.5f + std::cos(angle) * radius;
                const float y = static_cast<float>(H) * 0.5f + std::sin(angle) * radius;

                fill(static_cast<int>(255 * t), 120, static_cast<int>(255 * (1.0f - t)));
                circle(x, y, 26.0f + 10.0f * std::sin(time * 3.0f + t * TWO_PI));
            }
        }
        popCanvas();

        applyEffect();

        fill(255);
        text(std::string("Effect: ") + effectName(mode), 20.0f, 30.0f);
        text("1 None  2 Blur  3 Grayscale  4 Invert  5 Threshold  6 Vignette  7 Pixelate", 20.0f, 55.0f);
    }

private:
    void applyEffect()
    {
        switch (mode) {
            case EffectMode::none:
                image(scene.colorTexture, 0, 0, W, H);
                break;
            case EffectMode::blur: {
                const float texelX = 1.0f / static_cast<float>(W);
                const float texelY = 1.0f / static_cast<float>(H);
                setUniform(blurShader, "u_TexelSize", uniform(texelX, texelY));
                setUniform(blurShader, "u_Radius", uniform(4.0f));

                // Separable gaussian blur: horizontal pass into blurScratch, then a
                // vertical pass reading blurScratch straight onto the window canvas.
                pushCanvas(blurScratch);
                shader(blurShader);
                setUniform(blurShader, "u_Direction", uniform(1.0f, 0.0f));
                image(scene.colorTexture, 0, 0, W, H);
                noShader();
                popCanvas();

                shader(blurShader);
                setUniform(blurShader, "u_Direction", uniform(0.0f, 1.0f));
                image(blurScratch.colorTexture, 0, 0, W, H);
                noShader();
                break;
            }
            case EffectMode::grayscale:
                shader(grayscaleShader);
                setUniform(grayscaleShader, "u_Amount", uniform(1.0f));
                image(scene.colorTexture, 0, 0, W, H);
                noShader();
                break;
            case EffectMode::invert:
                shader(invertShader);
                setUniform(invertShader, "u_Amount", uniform(1.0f));
                image(scene.colorTexture, 0, 0, W, H);
                noShader();
                break;
            case EffectMode::threshold:
                shader(thresholdShader);
                setUniform(thresholdShader, "u_Amount", uniform(0.5f));
                image(scene.colorTexture, 0, 0, W, H);
                noShader();
                break;
            case EffectMode::vignette:
                shader(vignetteShader);
                setUniform(vignetteShader, "u_Strength", uniform(1.4f + 0.3f * std::sin(time)));
                image(scene.colorTexture, 0, 0, W, H);
                noShader();
                break;
            case EffectMode::pixelate:
                shader(pixelateShader);
                setUniform(pixelateShader, "u_BlockSize", uniform(4.0f + 4.0f * (0.5f + 0.5f * std::sin(time * 0.7f))));
                setUniform(pixelateShader, "u_TexelSize", uniform(1.0f / static_cast<float>(W), 1.0f / static_cast<float>(H)));
                image(scene.colorTexture, 0, 0, W, H);
                noShader();
                break;
        }
    }

    EffectMode mode = EffectMode::none;
    float time = 0.0f;

    Framebuffer scene;
    Framebuffer blurScratch;

    Shader blurShader;
    Shader grayscaleShader;
    Shader invertShader;
    Shader thresholdShader;
    Shader vignetteShader;
    Shader pixelateShader;
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<CustomEffectsSketch>();
    }
} // namespace p5cpp
