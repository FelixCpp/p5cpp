#include <p5cpp/p5cpp.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

using namespace p5cpp;

// HSV → RGBA (h: 0–360, s/v: 0–1)
static color_t hsv(float h, float s, float v, int a = 255)
{
    h = std::fmod(h, 360.f);
    const float c = v * s;
    const float x = c * (1.f - std::abs(std::fmod(h / 60.f, 2.f) - 1.f));
    const float m = v - c;
    float r, g, b;
    if (h < 60) {
        r = c;
        g = x;
        b = 0;
    } else if (h < 120) {
        r = x;
        g = c;
        b = 0;
    } else if (h < 180) {
        r = 0;
        g = c;
        b = x;
    } else if (h < 240) {
        r = 0;
        g = x;
        b = c;
    } else if (h < 300) {
        r = x;
        g = 0;
        b = c;
    } else {
        r = c;
        g = 0;
        b = x;
    }
    return rgba((int)((r + m) * 255), (int)((g + m) * 255), (int)((b + m) * 255), a);
}

struct Particle
{
    float2 pos;
    float2 vel;
    float life;
    float maxLife;
    float hue;
    float size;
};

static void burst(std::vector<Particle>& out, float cx, float cy, int count, float minSpeed, float maxSpeed, float baseHue)
{
    for (int i = 0; i < count; ++i) {
        const float angle = TWO_PI * i / count + randomFloat(-0.3f, 0.3f);
        const float speed = randomFloat(minSpeed, maxSpeed);
        const float life = randomFloat(1.0f, 2.5f);
        out.push_back({
            .pos = {cx, cy},
            .vel = float2::fromAngle(angle) * speed,
            .life = life,
            .maxLife = life,
            .hue = baseHue + angle * (180.f / PI),
            .size = randomFloat(3.f, 9.f),
        });
    }
}

struct GalaxySketch : Sketch
{
    std::vector<Particle> particles;
    float hueOffset = 0.f;

    void setup() override
    {
        setWindowSize(900, 700);
        setWindowTitle("p5cpp – Galaxy Vortex  |  Move mouse · Click to explode · ESC to quit");
        frameRate(60);
        particles.reserve(8000);
    }

    void draw() override
    {
        background(4, 4, 12, 18); // semi-transparent clear → motion blur trail

        const float dt = getDeltaTime();
        const float cx = (float)getMouseX();
        const float cy = (float)getMouseY();
        hueOffset += dt * 50.f;

        // Continuous vortex emitter around the cursor
        for (int i = 0; i < 10; ++i) {
            const float angle = randomFloat(0.f, TWO_PI);
            const float spread = randomFloat(0.f, 20.f);
            const float speed = randomFloat(100.f, 320.f);
            const float life = randomFloat(1.2f, 3.0f);

            // tangential kick creates the swirl
            float2 dir = float2::fromAngle(angle);
            float2 tangent = dir.perpendicular();
            float2 vel = dir * speed + tangent * randomFloat(-80.f, 80.f);
            vel.y -= randomFloat(10.f, 40.f); // slight upward bias

            particles.push_back({
                .pos = {cx + dir.x * spread, cy + dir.y * spread},
                .vel = vel,
                .life = life,
                .maxLife = life,
                .hue = hueOffset + angle * (180.f / PI),
                .size = randomFloat(5.f, 15.f),
            });
        }

        // Additive blending → overlapping particles bloom and glow
        blendMode(BlendMode::additive);
        noStroke();

        for (auto& p : particles) {
            p.vel.y += 120.f * dt; // gravity
            p.vel *= 0.993f;       // air drag
            p.pos += p.vel * dt;
            p.life -= dt;

            const float t = p.life / p.maxLife; // 1 → 0
            const float sz = p.size * t;
            const int a = (int)(t * t * 210.f);
            fill(hsv(p.hue, 1.f, 1.f, a));
            circle(p.pos.x, p.pos.y, sz);
        }

        blendMode(BlendMode::alpha);
        std::erase_if(particles, [](const Particle& p) {
            return p.life <= 0.f;
        });
    }

    void event(const WindowEvent& e) override
    {
        if (e.type == EventType::mousePress)
            burst(particles, (float)getMouseX(), (float)getMouseY(), 160, 150.f, 600.f, hueOffset);

        if (e.type == EventType::keyPress && e.keyEvent.key == Key::escape)
            quit();
    }
};

std::unique_ptr<Sketch> p5cpp::createSketch()
{
    return std::make_unique<GalaxySketch>();
}
