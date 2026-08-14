#include <p5cpp/p5cpp.hpp>

#include <cstdlib>
#include <ctime>

using namespace p5;

inline static constexpr std::string_view effectVertexShaderSource = R"(
            #version 410

            layout (location = 0) in vec2 a_Position;
            layout (location = 1) in vec2 a_TexCoord;
            layout (location = 2) in vec4 a_Color;

            uniform mat4 u_ProjectionMatrix;

            out vec2 v_TexCoord;
            out vec4 v_Color;

            void main()
            {
                gl_Position = u_ProjectionMatrix * vec4(a_Position, 0.0, 1.0);
                v_TexCoord = a_TexCoord;
                v_Color = a_Color;
            }
        )";

inline static constexpr std::string_view effectFragmentShaderSource = R"(
    #version 410

    layout (location = 0) out vec4 o_FragColor;

    in vec2 v_TexCoord;
    in vec4 v_Color;

    uniform sampler2D u_Texture;

    // ------------------------------------------------------------
    // Time
    // ------------------------------------------------------------

    uniform float u_Time;


    // ------------------------------------------------------------
    // Flash
    // 0.0 = normal
    // 1.0 = completely white
    // ------------------------------------------------------------

    uniform float u_Flash;


    // ------------------------------------------------------------
    // Pulse
    // 0.0 = disabled
    // ------------------------------------------------------------

    uniform float u_Pulse;


    // ------------------------------------------------------------
    // Wave / distortion
    // ------------------------------------------------------------

    uniform float u_Wave;
    uniform float u_WaveFrequency;
    uniform float u_WaveSpeed;


    // ------------------------------------------------------------
    // RGB split / chromatic aberration
    // ------------------------------------------------------------

    uniform float u_RGBSplit;


    // ------------------------------------------------------------
    // Hue shift
    // ------------------------------------------------------------

    uniform float u_Hue;


    // ------------------------------------------------------------
    // Dissolve
    //
    // 0.0 = completely visible
    // 1.0 = completely dissolved
    // ------------------------------------------------------------

    uniform float u_Dissolve;


    // ------------------------------------------------------------
    // Simple procedural noise
    // ------------------------------------------------------------

    float hash(vec2 p)
    {
        p = fract(p * vec2(123.34, 456.21));
        p += dot(p, p + 45.32);

        return fract(p.x * p.y);
    }


    float noise(vec2 p)
    {
        vec2 i = floor(p);
        vec2 f = fract(p);

        f = f * f * (3.0 - 2.0 * f);

        float a = hash(i);
        float b = hash(i + vec2(1.0, 0.0));
        float c = hash(i + vec2(0.0, 1.0));
        float d = hash(i + vec2(1.0, 1.0));

        return mix(
            mix(a, b, f.x),
            mix(c, d, f.x),
            f.y
        );
    }


    // ------------------------------------------------------------
    // RGB -> HSV
    // ------------------------------------------------------------

    vec3 rgbToHsv(vec3 c)
    {
        vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);

        vec4 p = mix(
            vec4(c.bg, K.wz),
            vec4(c.gb, K.xy),
            step(c.b, c.g)
        );

        vec4 q = mix(
            vec4(p.xyw, c.r),
            vec4(c.r, p.yzx),
            step(p.x, c.r)
        );

        float d = q.x - min(q.w, q.y);
        float e = 1.0e-10;

        return vec3(
            abs(q.z + (q.w - q.y) / (6.0 * d + e)),
            d / (q.x + e),
            q.x
        );
    }


    // ------------------------------------------------------------
    // HSV -> RGB
    // ------------------------------------------------------------

    vec3 hsvToRgb(vec3 c)
    {
        vec3 p = abs(
            fract(c.xxx + vec3(0.0, 1.0 / 3.0, 2.0 / 3.0)) * 6.0
            - 3.0
        );

        return c.z * mix(
            vec3(1.0),
            clamp(p - 1.0, 0.0, 1.0),
            c.y
        );
    }


    void main()
    {
        vec2 uv = v_TexCoord;


        // --------------------------------------------------------
        // Wave distortion
        // --------------------------------------------------------

        if (u_Wave > 0.0)
        {
            uv.x += sin(
                uv.y * u_WaveFrequency
                + u_Time * u_WaveSpeed
            ) * u_Wave;

            uv.y += cos(
                uv.x * u_WaveFrequency
                + u_Time * u_WaveSpeed
            ) * u_Wave * 0.5;
        }


        // --------------------------------------------------------
        // RGB split
        // --------------------------------------------------------

        vec4 color;

        if (u_RGBSplit > 0.0)
        {
            float offset = u_RGBSplit;

            float r = texture(
                u_Texture,
                uv + vec2(offset, 0.0)
            ).r;

            float g = texture(
                u_Texture,
                uv
            ).g;

            float b = texture(
                u_Texture,
                uv - vec2(offset, 0.0)
            ).b;

            float a = texture(u_Texture, uv).a;

            color = vec4(r, g, b, a);
        }
        else
        {
            color = texture(u_Texture, uv);
        }


        color *= v_Color;


        // --------------------------------------------------------
        // Hue shift
        // --------------------------------------------------------

        if (u_Hue != 0.0)
        {
            vec3 hsv = rgbToHsv(color.rgb);

            hsv.x = fract(hsv.x + u_Hue);

            color.rgb = hsvToRgb(hsv);
        }


        // --------------------------------------------------------
        // Pulse
        // --------------------------------------------------------

        if (u_Pulse > 0.0)
        {
            float pulse = 1.0
                + sin(u_Time * 6.28318) * u_Pulse;

            color.rgb *= pulse;
        }


        // --------------------------------------------------------
        // Flash
        // --------------------------------------------------------

        if (u_Flash > 0.0)
        {
            color.rgb = mix(
                color.rgb,
                vec3(1.0),
                u_Flash
            );
        }


        // --------------------------------------------------------
        // Dissolve
        // --------------------------------------------------------

        if (u_Dissolve > 0.0)
        {
            float n = noise(uv * 12.0);

            if (n < u_Dissolve)
                discard;
        }


        o_FragColor = color;
    }
)";

float linear(float x) { return x; }
float easeInOutSine(float x) { return -(cos(3.14159265358979323846 * x) - 1.0f) / 2.0f; }
float easeOutQuad(float x) { return 1.0f - (1.0f - x) * (1.0f - x); }

struct Animation
{
    float from;
    float to;
    float duration;
    float elapsedTime;
    float (*ease)(float) = nullptr;
    bool isPlaying;
};

Animation animation(float from, float to, float duration, float (*ease)(float) = &easeInOutSine)
{
    return Animation {
        .from = from,
        .to = to,
        .duration = duration,
        .elapsedTime = 0.0f,
        .ease = ease,
        .isPlaying = false,
    };
}

void advance(Animation& animation, float deltaTime)
{
    if (not animation.isPlaying) {
        return;
    }

    animation.elapsedTime = constrain(animation.elapsedTime + deltaTime, 0.0f, animation.duration);
    animation.isPlaying = animation.elapsedTime < animation.duration;
}

float value(const Animation& animation)
{
    const float safeDuration = animation.duration > 0.0f ? animation.duration : 1.0f;
    const float progress = animation.elapsedTime / safeDuration;
    const float easedProgress = animation.ease ? animation.ease(progress) : linear(progress);
    const float value = lerp(animation.from, animation.to, easedProgress);

    return value;
}

void resume(Animation& animation)
{
    animation.isPlaying = true;
}

void pause(Animation& animation)
{
    animation.isPlaying = false;
}

void stop(Animation& animation)
{
    animation.isPlaying = false;
    animation.elapsedTime = 0.0f;
}

void reset(Animation& animation)
{
    animation.isPlaying = false;
    animation.elapsedTime = 0.0f;
}

void restart(Animation& animation)
{
    animation.isPlaying = true;
    animation.elapsedTime = 0.0f;
}

bool isPlaying(const Animation& animation)
{
    return animation.isPlaying;
}

bool isPaused(const Animation& animation)
{
    return not animation.isPlaying and animation.elapsedTime < animation.duration;
}

bool isFinished(const Animation& animation)
{
    return not animation.isPlaying and animation.elapsedTime >= animation.duration;
}

struct Shake
{
    float duration;
    float elapsedTime;
    float magnitude;
    bool isShaking;
};

Shake shake(float duration, float magnitude)
{
    return Shake {
        .duration = duration,
        .elapsedTime = 0.0f,
        .magnitude = magnitude,
        .isShaking = false,
    };
}

void advance(Shake& shake, float deltaTime)
{
    if (not shake.isShaking) {
        return;
    }

    shake.elapsedTime = constrain(shake.elapsedTime + deltaTime, 0.0f, shake.duration);
    shake.isShaking = shake.elapsedTime < shake.duration;
}

float2 value(const Shake& shake)
{
    if (not shake.isShaking) {
        return {0.0f, 0.0f};
    }

    const float progress = shake.elapsedTime / shake.duration;
    const float decay = 1.0f - easeOutQuad(progress);

    const float offsetX = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * shake.magnitude * decay;
    const float offsetY = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * shake.magnitude * decay;

    return {offsetX, offsetY};
}

struct EffectsSketch : public Sketch
{
    Animation flash = animation(0.0f, 1.0f, 0.15f);
    Animation dissolve = animation(0.0f, 1.0f, 1.0f);
    Animation rgbSplit = animation(0.0f, 0.02f, 0.3f);
    Animation wave = animation(0.0f, 0.02f, 0.4f);
    Shake cameraShake = shake(0.5f, 10.0f);

    std::shared_ptr<Shader> effectShader = loadShaderFromMemory(effectVertexShaderSource, effectFragmentShaderSource);
    std::shared_ptr<Texture> texture = loadTextureFromFile("platformer.jpg");

    void setup() override
    {
        setWindowSize(800, 600);
    }

    void draw() override
    {
        background(0);

        if (isKeyPressed(Key::F)) {
            reset(dissolve);
            restart(flash);
        }

        if (isKeyPressed(Key::D))
            restart(dissolve);

        if (isKeyPressed(Key::R)) {
            reset(dissolve);
            restart(rgbSplit);
        }

        if (isKeyPressed(Key::W))
            restart(wave);

        if (isKeyPressed(Key::Space)) {
            stop(flash);
            stop(dissolve);
            stop(rgbSplit);
            stop(wave);
        }

        fprintf(stdout, "Flash: %f, Dissolve: %f, RGB Split: %f, Wave: %f\n", value(flash), value(dissolve), value(rgbSplit), value(wave));
        fflush(stdout);

        setUniform(*effectShader, "u_Flash", value(flash));
        setUniform(*effectShader, "u_Dissolve", value(dissolve));
        setUniform(*effectShader, "u_RGBSplit", value(rgbSplit));
        setUniform(*effectShader, "u_Time", getGlobalTime());
        setUniform(*effectShader, "u_Wave", value(wave));
        setUniform(*effectShader, "u_WaveFrequency", 30.0f);
        setUniform(*effectShader, "u_WaveSpeed", 4.0f);

        shader(effectShader);
        image(texture, 100, 100, 256, 256);
        noShader();

        advance(flash, getDeltaTime());
        advance(dissolve, getDeltaTime());
        advance(rgbSplit, getDeltaTime());
        advance(wave, getDeltaTime());
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<EffectsSketch>();
}
