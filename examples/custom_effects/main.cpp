// custom_effects.cpp
// -----------------------------------------------------------------------
// Demonstrates p5cpp's post-processing effect API:
//
//   - filter(FilterType, amount) for the built-in effects (blur, grayscale,
//     invert, threshold).
//   - loadEffectShader(): a lightweight way to write your OWN full-screen
//     effect, in addition to the built-ins.
//
// A custom effect is just a GLSL snippet that defines:
//
//     vec4 effect(vec2 uv, vec2 texelSize, sampler2D tex)
//
// loadEffectShader() wraps that snippet with the vertex shader and texture
// sampling boilerplate a full-screen pass needs, so callers never have to
// write either by hand. The resulting Shader is applied every frame with
// effect(shader) - the built-in filters (see internal_shaders.cpp) are
// implemented with the very same helper.
//
// Steuerung:
//   1 - kein Effekt
//   2 - Blur       (built-in filter)
//   3 - Grayscale  (built-in filter)
//   4 - Invert     (built-in filter)
//   5 - Threshold  (built-in filter)
//   6 - Vignette   (custom effect via loadEffectShader)
//   7 - Pixelate   (custom effect via loadEffectShader)
//   Escape - beenden
// -----------------------------------------------------------------------

#include <p5cpp/p5cpp.hpp>

#include <cmath>
#include <string>
#include <vector>

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
            case EffectMode::blur: return "Blur (built-in filter)";
            case EffectMode::grayscale: return "Grayscale (built-in filter)";
            case EffectMode::invert: return "Invert (built-in filter)";
            case EffectMode::threshold: return "Threshold (built-in filter)";
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
    void setup() override
    {
        setWindowSize(960, 720);
        setWindowTitle("p5cpp - Custom Effects");
        frameRate(60);

        vignetteShader = Shader(std::shared_ptr<ShaderImpl>(loadEffectShader(vignetteSource).release()));
        pixelateShader = Shader(std::shared_ptr<ShaderImpl>(loadEffectShader(pixelateSource).release()));
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

        background(18, 18, 24);

        noStroke();
        const int shapeCount = 24;
        for (int i = 0; i < shapeCount; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(shapeCount);
            const float angle = t * TWO_PI + time * 0.6f;
            const float radius = 180.0f + 60.0f * std::sin(time * 0.8f + t * TWO_PI * 3.0f);

            const float x = static_cast<float>(getLogicalWidth()) * 0.5f + std::cos(angle) * radius;
            const float y = static_cast<float>(getLogicalHeight()) * 0.5f + std::sin(angle) * radius;

            fill(static_cast<int>(255 * t), 120, static_cast<int>(255 * (1.0f - t)));
            circle(x, y, 26.0f + 10.0f * std::sin(time * 3.0f + t * TWO_PI));
        }

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
                break;
            case EffectMode::blur:
                filter(FilterType::blur, 4.0f);
                break;
            case EffectMode::grayscale:
                filter(FilterType::grayscale, 1.0f);
                break;
            case EffectMode::invert:
                filter(FilterType::invert, 1.0f);
                break;
            case EffectMode::threshold:
                filter(FilterType::threshold, 0.5f);
                break;
            case EffectMode::vignette:
                setUniform(vignetteShader, "u_Strength", uniform(1.4f + 0.3f * std::sin(time)));
                effect(vignetteShader);
                break;
            case EffectMode::pixelate:
                setUniform(pixelateShader, "u_BlockSize", uniform(4.0f + 4.0f * (0.5f + 0.5f * std::sin(time * 0.7f))));
                effect(pixelateShader);
                break;
        }
    }

    EffectMode mode = EffectMode::none;
    float time = 0.0f;

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
