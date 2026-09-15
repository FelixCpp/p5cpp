#include <p5cpp/p5cpp.hpp>

using namespace p5;

namespace
{
    const std::string_view lightningShaderSource = R"(
        struct Uniforms {
            u_Start: vec2f,
            u_End: vec2f,
            u_Time: f32,
            u_Seed: f32,
        };
        @group(2) @binding(0) var<uniform> u_Extra: Uniforms;

        fn hash(p: f32) -> f32 { return fract(sin(p * 127.1 + u_Extra.u_Seed) * 43758.5453); }

        fn effect(color: vec4f, image: texture_2d<f32>, imageSampler: sampler, texCoord: vec2f, screenCoord: vec2f) -> vec4f {
            let dir = normalize(u_Extra.u_End - u_Extra.u_Start);
            let normal = vec2f(-dir.y, dir.x);
            let toPoint = screenCoord - u_Extra.u_Start;
            let along = dot(toPoint, dir);
            let perpDist = dot(toPoint, normal);

            let jitter = (hash(along * 0.05 + floor(u_Extra.u_Time * 20.0)) - 0.5) * 1.0;
            let core = smoothstep(3.0, 0.0, abs(perpDist - jitter));
            let glow = smoothstep(20.0, 0.0, abs(perpDist - jitter));

            return vec4f(0.75, 0.88, 1.0, 1.0) * (core + glow * 0.35) * color.a;
        }
    )";

    const std::string_view screenFlashShaderSource = R"(
        struct Uniforms {
            u_FlashAmount: f32,
        };
        @group(2) @binding(0) var<uniform> u_Extra: Uniforms;

        fn effect(color: vec4f, image: texture_2d<f32>, imageSampler: sampler, texCoord: vec2f, screenCoord: vec2f) -> vec4f {
            let sceneColor = textureSample(image, imageSampler, texCoord);
            return mix(sceneColor, vec4f(1.0, 1.0, 1.0, 1.0), u_Extra.u_FlashAmount) * color;
        }
    )";

    struct Bolt
    {
        float2 from;
        float2 to;
        double startTime;
    };
} // namespace

struct ChainLightning : Sketch
{
    Graphics sceneBuffer;
    Shader lightningShader = loadShaderFromMemory(lightningShaderSource).value();
    Shader flashShader = loadShaderFromMemory(screenFlashShaderSource).value();

    std::vector<float2> enemies = {
        {300, 200},
        {380, 260},
        {260, 320},
        {420, 380},
        {520, 260},
    };

    std::vector<Bolt> bolts;
    double flashTriggeredAt = -1000.0;

    void setup() override
    {
        sceneBuffer = createGraphics(getWidth(), getHeight()).value();
    }

    void triggerChain(float2 origin)
    {
        std::vector<float2> remaining = enemies;
        float2 current = origin;
        const float jumpRadius = 220.0f;
        const int maxJumps = 3;

        for (int jump = 0; jump < maxJumps && not remaining.empty(); ++jump) {
            auto nearestIt = std::min_element(remaining.begin(), remaining.end(), [&](const float2& a, const float2& b) {
                return distance(a, current) < distance(b, current);
            });
            if (distance(*nearestIt, current) > jumpRadius) break;

            bolts.push_back({current, *nearestIt, getGlobalTime() + jump * 0.08});
            current = *nearestIt;
            remaining.erase(nearestIt);
        }

        flashTriggeredAt = getGlobalTime();
    }

    void drawBolt(const Bolt& bolt)
    {
        const float2 dir = normalized(bolt.to - bolt.from);
        const float2 normal = perpendicular(dir);
        const float margin = 24.0f;

        withShader(lightningShader, [&]() {
            setUniform("u_Start", bolt.from);
            setUniform("u_End", bolt.to);
            setUniform("u_Time", static_cast<float>(getGlobalTime() - bolt.startTime));
            setUniform("u_Seed", bolt.from.x + bolt.from.y);

            beginShape(ShapeMode::quads);
            vertex((bolt.from + normal * margin - dir * margin).x, (bolt.from + normal * margin - dir * margin).y);
            vertex((bolt.to + normal * margin + dir * margin).x, (bolt.to + normal * margin + dir * margin).y);
            vertex((bolt.to - normal * margin + dir * margin).x, (bolt.to - normal * margin + dir * margin).y);
            vertex((bolt.from - normal * margin - dir * margin).x, (bolt.from - normal * margin - dir * margin).y);
            endShape();
        });
    }

    void draw() override
    {
        if (isMouseButtonPressed(MouseButton::left)) {
            triggerChain({static_cast<float>(getMouseX()), static_cast<float>(getMouseY())});
        }

        std::erase_if(bolts, [&](const Bolt& b) {
            return getGlobalTime() - b.startTime > 0.4;
        });

        withGraphics(sceneBuffer, [&]() {
            background(rgba(15, 15, 25));

            noStroke();
            fill(rgba(200, 60, 60));
            for (const float2& e : enemies) {
                circle(e.x, e.y, 20);
            }

            for (const Bolt& bolt : bolts) {
                if (getGlobalTime() >= bolt.startTime) {
                    drawBolt(bolt);
                }
            }
        });

        const float flashElapsed = static_cast<float>(getGlobalTime() - flashTriggeredAt);
        const float flashAmount = std::max(0.0f, 1.0f - flashElapsed * 6.0f);

        withShader(flashShader, [&]() {
            setUniform("u_FlashAmount", flashAmount);
            image(sceneBuffer.colorTexture, 0, 0, static_cast<float>(getWidth()), static_cast<float>(getHeight()));
        });
    }
};

SketchSpec p5::createSpec()
{
    return {.sketch = [] {
        return std::make_unique<ChainLightning>();
    }};
}
