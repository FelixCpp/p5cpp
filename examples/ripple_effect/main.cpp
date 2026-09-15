#include <p5cpp/p5cpp.hpp>

using namespace p5;

inline static constexpr std::string_view RIPPLE_SHADER_SOURCE = R"(
struct Uniforms {
    u_Center: vec2f,
    u_Time: f32,
    u_Aspect: f32,
};

@group(2) @binding(0) var<uniform> u_Extra: Uniforms;

fn effect(color: vec4f, image: texture_2d<f32>, imageSampler: sampler, texCoord: vec2f, screenCoord: vec2f) -> vec4f {
    var d = texCoord - u_Extra.u_Center;
    d.x *= u_Extra.u_Aspect;
    let dist = length(d);

    let speed = 4.0;
    let frequency = 40.0;
    let amplitude = 0.015;
    let decay = 2.0;

    let wave = sin(dist * frequency - u_Extra.u_Time * speed);
    let falloff = exp(-u_Extra.u_Time * decay) * exp(-dist * 3.0);
    let offset = normalize(d + vec2(0.0001)) * wave * amplitude * falloff;
    return textureSample(image, imageSampler, texCoord + offset) * color;
}
)";

struct RippleEffect : public Sketch
{
    Texture mountainsTexture;
    Shader rippleShader;

    bool isRippleActive = false;
    float rippleTime = 0.0f;
    float2 rippleCenter;

    void setup() override
    {
        setWindowSize(800, 600);
        mountainsTexture = loadTexture("assets/Mountains.jpeg").value();
        rippleShader = loadShaderFromMemory(RIPPLE_SHADER_SOURCE).value();
    }

    void draw() override
    {
        background(rgba(31, 31, 51));

        const float width = static_cast<float>(getWidth());
        const float height = static_cast<float>(getHeight());

        if (isMouseButtonPressed(MouseButton::left)) {
            isRippleActive = true;
            rippleTime = getGlobalTime();
            rippleCenter = float2 {
                .x = static_cast<float>(getMouseX()) / width,
                .y = static_cast<float>(getMouseY()) / height,
            };
        }

        if (not isRippleActive) {
            image(mountainsTexture, 0, 0, width, height);
        } else {
            withShader(rippleShader, [&] {
                const float aspectRatio = width / height;
                const float time = getGlobalTime() - rippleTime;

                setUniform("u_Center", rippleCenter);
                setUniform("u_Time", time);
                setUniform("u_Aspect", aspectRatio);
                image(mountainsTexture, 0, 0, width, height);
            });
        }
    }
};

SketchSpec p5::createSpec()
{
    return SketchSpec {
        .sketch = [] {
            return std::make_unique<RippleEffect>();
        }
    };
}
