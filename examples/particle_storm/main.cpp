#include <p5cpp/p5cpp.hpp>

#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <numbers>
#include <vector>

using namespace p5;

namespace
{
    struct Particle
    {
        float2 position;
        float2 velocity;
        float hue; // 0..1
    };

    // p5cpp only exposes RGB color construction, so a small HSV->RGB helper gets us a smooth,
    // vibrant hue cycle without hand-picking a palette.
    color_t hsv(float h, float s, float v, int32_t alpha = 255)
    {
        h -= std::floor(h);
        const float i = std::floor(h * 6.0f);
        const float f = h * 6.0f - i;
        const float p = v * (1.0f - s);
        const float q = v * (1.0f - f * s);
        const float t = v * (1.0f - (1.0f - f) * s);

        float r, g, b;
        switch (static_cast<int>(i) % 6) {
            case 0:
                r = v;
                g = t;
                b = p;
                break;
            case 1:
                r = q;
                g = v;
                b = p;
                break;
            case 2:
                r = p;
                g = v;
                b = t;
                break;
            case 3:
                r = p;
                g = q;
                b = v;
                break;
            case 4:
                r = t;
                g = p;
                b = v;
                break;
            default:
                r = v;
                g = p;
                b = q;
                break;
        }
        return rgba(static_cast<int32_t>(r * 255.0f), static_cast<int32_t>(g * 255.0f), static_cast<int32_t>(b * 255.0f), alpha);
    }

    // A slowly drifting Perlin-noise angle field — the classic "flow field" look.
    float2 flowDirection(float x, float y, float t)
    {
        const float angle = noise(x * 0.0035f, y * 0.0035f, t * 0.06f) * 4.0f * std::numbers::pi_v<float>;
        return {std::cos(angle), std::sin(angle)};
    }
} // namespace

struct ParticleStormSketch : public Sketch
{
    void setup() override
    {
        setWindowSize(1200, 800);
        setWindowTitle("Particle Storm");

        noiseDetail(4, 0.5f);

        m_particles.reserve(kMaxParticles);
        respawnTo(kBaseParticles);
    }

    void draw() override
    {
        updateParticleTarget();
        updateParticles();
        renderFrame();
        writePerfLogIfDue();
    }

private:
    static constexpr size_t kBaseParticles = 1800;
    static constexpr size_t kPeakParticles = 11000;
    static constexpr size_t kMaxParticles = 12000;
    static constexpr float kOscillationPeriodSeconds = 14.0f;
    static constexpr float kBurstPeriodSeconds = 4.0f;
    static constexpr float kFlowSpeed = 90.0f;

    void updateParticleTarget()
    {
        // A slow sine wave between kBaseParticles and kPeakParticles gives the CPU/memory graphs
        // a clean, readable oscillation instead of a flat line.
        const float t = static_cast<float>(getGlobalTime());
        const float wave = 0.5f + 0.5f * std::sin(t * (2.0f * std::numbers::pi_v<float> / kOscillationPeriodSeconds));
        const size_t target = kBaseParticles + static_cast<size_t>(wave * static_cast<float>(kPeakParticles - kBaseParticles));
        respawnTo(target);

        // Periodic bursts on top of the wave: a short-lived crowd of particles radiating from the
        // center, for a visible spike plus a distinct "shockwave" visual moment.
        m_burstAccumulator += getDeltaTime();
        if (m_burstAccumulator >= kBurstPeriodSeconds) {
            m_burstAccumulator = 0.0;
            spawnBurst(1200);
        }
    }

    void respawnTo(size_t target)
    {
        target = std::min(target, kMaxParticles);
        const uint2 winSize = getWindowSize();
        while (m_particles.size() < target) {
            m_particles.push_back(Particle {
                .position = {random(static_cast<float>(winSize.x)), random(static_cast<float>(winSize.y))},
                .velocity = {0.0f, 0.0f},
                .hue = random(1.0f),
            });
        }
        if (m_particles.size() > target) m_particles.resize(target);
    }

    void spawnBurst(size_t count)
    {
        const uint2 winSize = getWindowSize();
        const float2 center = {static_cast<float>(winSize.x) * 0.5f, static_cast<float>(winSize.y) * 0.5f};

        for (size_t i = 0; i < count and m_particles.size() < kMaxParticles; ++i) {
            const float angle = random(2.0f * std::numbers::pi_v<float>);
            const float speed = random(60.0f, 220.0f);
            m_particles.push_back(Particle {
                .position = center,
                .velocity = {std::cos(angle) * speed, std::sin(angle) * speed},
                .hue = m_burstHue,
            });
        }
        m_burstHue = std::fmod(m_burstHue + 0.18f, 1.0f);
    }

    void updateParticles()
    {
        const uint2 winSize = getWindowSize();
        const float w = static_cast<float>(winSize.x);
        const float h = static_cast<float>(winSize.y);
        const float dt = static_cast<float>(getDeltaTime());
        const float t = static_cast<float>(getGlobalTime());

        for (Particle& particle : m_particles) {
            const float2 flow = flowDirection(particle.position.x, particle.position.y, t);
            particle.velocity.x = lerp(particle.velocity.x, flow.x * kFlowSpeed, 0.08f);
            particle.velocity.y = lerp(particle.velocity.y, flow.y * kFlowSpeed, 0.08f);

            particle.position.x += particle.velocity.x * dt;
            particle.position.y += particle.velocity.y * dt;

            // Wrap at the edges so the flow reads as an endless field rather than emptying out.
            if (particle.position.x < 0.0f) particle.position.x += w;
            if (particle.position.x > w) particle.position.x -= w;
            if (particle.position.y < 0.0f) particle.position.y += h;
            if (particle.position.y > h) particle.position.y -= h;
        }
    }

    void renderFrame()
    {
        const uint2 winSize = getWindowSize();

        // A low-alpha fade instead of a hard clear leaves a glowing trail behind every particle.
        // Nothing clears the framebuffer between frames on its own, so this is what makes it fade.
        blendMode(BlendMode::alpha);
        noStroke();
        fill(rgba(6, 6, 10, 26));
        rect(0.0f, 0.0f, static_cast<float>(winSize.x), static_cast<float>(winSize.y));

        // Additive blending makes overlapping particles glow brighter — denser flow regions light up.
        blendMode(BlendMode::additive);
        strokeWeight(2.4f);
        // Draw state doesn't carry over from setup() into draw() (each Sketch lifecycle callback
        // runs inside its own push()/pop() bracket in GraphicsPlugin), so this has to be set here,
        // every frame, not once in setup(). Default StrokeCap is round, which makes point()
        // tessellate a full heap-allocated circle fan per call; square caps make it a plain quad
        // instead — the difference between ~6 and ~48+ indices per particle, which is exactly
        // what blew through the renderer's per-frame index budget before this was moved here.
        strokeCap(StrokeCap::square);
        const float t = static_cast<float>(getGlobalTime());
        for (const Particle& particle : m_particles) {
            stroke(hsv(particle.hue + t * 0.02f, 0.75f, 1.0f, 160));
            point(particle.position.x, particle.position.y);
        }

        blendMode(BlendMode::alpha);
        drawHud();
    }

    void drawHud()
    {
        noStroke();
        fill(rgba(255, 255, 255, 150));
        textSize(12);
        textAlign(TextAlignment::bottomLeft);
        text(std::format("{} particles · {:.0f} fps", m_particles.size(), 1.0 / std::max(getDeltaTime(), 1e-6)), 12.0f, static_cast<float>(getWindowSize().y) - 10.0f);
    }

    void writePerfLogIfDue()
    {
        // Deliberate, bounded disk I/O so a profiler watching this process has something to show on
        // a Disk I/O graph too — always overwrites the same small file rather than growing forever.
        m_logAccumulator += getDeltaTime();
        if (m_logAccumulator < 1.0) return;
        m_logAccumulator = 0.0;

        std::ofstream log(std::filesystem::temp_directory_path() / "particle_storm_stats.log", std::ios::trunc);
        log << std::format("frame={} particles={} fps={:.1f}\n", getFrameCount(), m_particles.size(), 1.0 / std::max(getDeltaTime(), 1e-6));
    }

    std::vector<Particle> m_particles;
    double m_burstAccumulator = 0.0;
    double m_logAccumulator = 0.0;
    float m_burstHue = 0.0f;
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<ParticleStormSketch>();
}
