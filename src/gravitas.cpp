#include "p5.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <string>
#include <vector>

using namespace p5;

// ─── Constants ────────────────────────────────────────────────────────────────
inline static constexpr float SPAWN_MARGIN = 100.0f;
inline static constexpr float PULL_RADIUS = 150.0f;
inline static constexpr float PULL_STRENGTH = 600.0f;
inline static constexpr float INFLUENCE_RADIUS = 100.0f;

// ─── Background Star ──────────────────────────────────────────────────────────
struct BackgroundStar
{
    float2 position;
    float baseSize;
    float noiseOffset;

    void show(float time) const
    {
        const float n = noise(noiseOffset, time * 0.4f);
        const float brightness = 0.2f + 0.8f * (n * n); // contrast curve → sparkle, not breathe
        const uint8_t a = static_cast<uint8_t>(brightness * 170.0f);
        noStroke();
        fill(rgba(200, 215, 255, a));
        circle(position.x, position.y, baseSize * 2.0f);
    }
};

// ─── Score Popup ──────────────────────────────────────────────────────────────
struct ScorePopup
{
    float2 position;
    float t = 0.0f;
    int value = 10;

    static constexpr float duration = 1.0f;

    bool alive() const { return t < 1.0f; }
    void update(float dt) { t = std::min(t + dt / duration, 1.0f); }

    void show() const
    {
        if (!alive()) return;
        const float fade = (1.0f - t) * (1.0f - t);
        fill(rgba(255, 220, 80, static_cast<uint8_t>(fade * 240.0f)));
        noStroke();
        textAlign(HorizontalTextAlign::center, VerticalTextAlign::center);
        textSize(22.0f);
        text("+" + std::to_string(value), position.x, position.y - t * 55.0f);
    }
};

// ─── Player ───────────────────────────────────────────────────────────────────
struct Player
{
    float2 position;
    float radius;
    float angle = 0.0f;

    explicit Player(float r) : position {0, 0}, radius(r) {}

    void update(float dt)
    {
        position = {static_cast<float>(mouseX), static_cast<float>(mouseY)};
        angle += dt * 1.4f;
    }

    void show() const
    {
        pushState();
        blendMode(BlendMode::additive);
        noFill();

        // Faint influence radius
        stroke(rgba(80, 130, 255, 14));
        strokeWeight(1.0f);
        circle(position.x, position.y, INFLUENCE_RADIUS * 2.0f);

        // Outer soft glow
        stroke(rgba(80, 150, 255, 20));
        strokeWeight(16.0f);
        circle(position.x, position.y, radius * 2.0f);

        // Mid glow
        stroke(rgba(120, 180, 255, 60));
        strokeWeight(6.0f);
        circle(position.x, position.y, radius * 2.0f);

        // Core ring
        stroke(rgba(200, 230, 255, 210));
        strokeWeight(1.5f);
        circle(position.x, position.y, radius * 2.0f);

        // 4 rotating arc segments
        stroke(rgba(160, 210, 255, 190));
        strokeWeight(2.0f);
        const float segLen = radians(50.0f);
        const float step = radians(90.0f);
        for (int i = 0; i < 4; ++i) {
            const float start = angle + i * step;
            // arc() takes width/height as full diameter
            arc(position.x, position.y, radius * 5.6f, radius * 5.6f, start, segLen, ArcMode::open);
        }

        // Bright center dot
        noStroke();
        fill(rgba(200, 230, 255, 200));
        circle(position.x, position.y, 4.0f);

        popState();
    }
};

// ─── Orbit ────────────────────────────────────────────────────────────────────
inline static constexpr int TRAIL_LEN = 24;
inline static constexpr float TRAIL_SAMPLE = 0.028f;

struct Orbit
{
    float2 position;
    float2 velocity;
    float2 acceleration;
    float maxSpeed = 300.0f;
    float minSpeed = 50.0f;
    float maxForce = 300.0f;
    float radius = 10.0f;
    float spawnT = 0.0f; // 0→1 spawn flash

    std::array<float2, TRAIL_LEN> trail {};
    int trailHead = 0;
    int trailCount = 0;
    float trailTimer = 0.0f;

    explicit Orbit(float2 pos, float2 vel)
        : position(pos), velocity(vel), acceleration {0, 0}
    {
        trail.fill(pos);
    }

    void attract(const float2& target, float dt)
    {
        const float2 toTarget = target - position;
        const float dist = length(toTarget);
        if (dist > INFLUENCE_RADIUS || dist < 1.0f) return;

        const float t = 1.0f - (dist / INFLUENCE_RADIUS);
        const float forceMag = maxForce * t * t;
        applyForce((toTarget / dist) * forceMag);
        velocity *= std::exp(-2.0f * t * dt);
    }

    void applyForce(const float2& f) { acceleration += f; }

    void update(float dt)
    {
        // Trail sampling — while loop avoids frame-rate dependence
        trailTimer += dt;
        while (trailTimer >= TRAIL_SAMPLE) {
            trail[trailHead] = position;
            trailHead = (trailHead + 1) % TRAIL_LEN;
            trailCount = std::min(trailCount + 1, TRAIL_LEN);
            trailTimer -= TRAIL_SAMPLE;
        }

        velocity += acceleration * dt;
        velocity = limit(velocity, maxSpeed);

        const float speed = length(velocity);
        if (speed < minSpeed)
            velocity = (speed > 0.001f ? velocity / speed : float2 {1, 0}) * minSpeed;

        acceleration = {0, 0};
        position += velocity * dt;

        spawnT = std::min(spawnT + dt * 2.5f, 1.0f);
    }

    void show() const
    {
        pushState();
        blendMode(BlendMode::additive);
        noStroke();

        // Trail — fading dots
        for (int i = 0; i < trailCount; ++i) {
            const int idx = (trailHead - 1 - i + TRAIL_LEN) % TRAIL_LEN;
            const float tf = 1.0f - static_cast<float>(i) / trailCount;
            const uint8_t a = static_cast<uint8_t>(tf * tf * 110.0f);
            fill(rgba(220, 140, 50, a));
            const float r = radius * 0.6f * tf;
            circle(trail[idx].x, trail[idx].y, r * 2.0f);
        }

        // Outer glow
        fill(rgba(230, 130, 40, 20));
        circle(position.x, position.y, radius * 4.2f);

        fill(rgba(235, 155, 55, 55));
        circle(position.x, position.y, radius * 2.6f);

        // Core
        fill(rgba(255, 195, 100, 195));
        circle(position.x, position.y, radius * 1.1f);

        // Hot center
        fill(rgba(255, 240, 200, 255));
        circle(position.x, position.y, radius * 0.4f);

        // Spawn flash ring (expands and fades in the first ~0.4s)
        if (spawnT < 1.0f) {
            const float sf = 1.0f - spawnT;
            noFill();
            stroke(rgba(255, 200, 100, static_cast<uint8_t>(sf * 200.0f)));
            strokeWeight(sf * 2.5f);
            circle(position.x, position.y, (radius + spawnT * 45.0f) * 2.0f);
        }

        popState();
    }
};

// ─── spawnOrbit ───────────────────────────────────────────────────────────────
Orbit spawnOrbit()
{
    const float randomX = random(static_cast<float>(width));
    const float randomY = random(static_cast<float>(height));

    const float2 orbitPosition = std::invoke([&]() {
        const int edge = static_cast<int>(random(4.0f));
        switch (edge) {
            case 0: return float2 {randomX, -SPAWN_MARGIN};
            case 1: return float2 {static_cast<float>(width) + SPAWN_MARGIN, randomY};
            case 2: return float2 {randomX, static_cast<float>(height) + SPAWN_MARGIN};
            default: return float2 {-SPAWN_MARGIN, randomY};
        }
    });

    const float2 oppositePoint {static_cast<float>(width) - randomX, static_cast<float>(height) - randomY};
    const float2 direction = normalized(oppositePoint - orbitPosition);
    return Orbit {orbitPosition, direction * random(60.0f, 160.0f)};
}

// ─── Star ─────────────────────────────────────────────────────────────────────
enum class StarType { Solar,
                      Nebula,
                      Pulsar };

struct Star
{
    float2 position;
    float radius;
    StarType type;
    float angle = 0.0f;
    float pulsePhase = 0.0f;

    explicit Star(float2 pos, float r, StarType t) : position(pos), radius(r), type(t)
    {
        pulsePhase = random(6.28318f);
    }

    bool contains(const Orbit& o) const
    {
        return lengthSquared(o.position - position) < radius * radius;
    }

    void update(float dt)
    {
        pulsePhase += dt * 2.2f;
        switch (type) {
            case StarType::Solar: angle += dt * 0.7f; break;
            case StarType::Nebula: angle += dt * 0.35f; break;
            case StarType::Pulsar: angle += dt * 4.0f; break; // fast beam rotation
        }
    }

    // ── Solar: warm orange sun — pulsing round corona + 4 rotating spikes ─────
    void showSolar() const
    {
        const float pulse = 0.82f + 0.18f * std::sin(pulsePhase);

        noStroke();
        fill(rgba(200, 60, 30, 10));
        circle(position.x, position.y, radius * 6.0f * pulse);
        fill(rgba(215, 80, 40, 28));
        circle(position.x, position.y, radius * 3.5f * pulse);
        fill(rgba(235, 110, 55, 70));
        circle(position.x, position.y, radius * 2.1f);
        fill(rgba(255, 155, 70, 200));
        circle(position.x, position.y, radius * 1.3f);
        fill(rgba(255, 230, 190, 255));
        circle(position.x, position.y, radius * 0.55f);

        noFill();
        stroke(rgba(255, 140, 60, 90));
        strokeWeight(1.5f);
        for (int i = 0; i < 4; ++i) {
            const float a = angle + i * radians(45.0f);
            const float r1 = radius * 1.4f;
            const float r2 = radius * 3.8f * pulse;
            line(position.x + std::cos(a) * r1, position.y + std::sin(a) * r1, position.x + std::cos(a) * r2, position.y + std::sin(a) * r2);
        }
    }

    // ── Nebula: emerald green — tilted orbital rings + 6 crystalline spikes ───
    void showNebula() const
    {
        const float pulse = 0.8f + 0.2f * std::sin(pulsePhase * 0.7f); // slow breathe

        noStroke();
        fill(rgba(20, 180, 90, 8));
        circle(position.x, position.y, radius * 7.5f * pulse);
        fill(rgba(35, 210, 110, 22));
        circle(position.x, position.y, radius * 4.2f * pulse);
        fill(rgba(55, 230, 140, 65));
        circle(position.x, position.y, radius * 2.3f);
        fill(rgba(90, 255, 160, 190));
        circle(position.x, position.y, radius * 1.3f);
        fill(rgba(200, 255, 225, 255));
        circle(position.x, position.y, radius * 0.5f);

        // Two tilted orbital rings (ellipses via matrix scale trick)
        noFill();
        stroke(rgba(60, 230, 145, 75));
        strokeWeight(1.2f);

        pushMatrix();
        translate(position.x, position.y);
        rotate(angle);
        scale(1.0f, 0.3f); // flatten to ellipse
        circle(0.0f, 0.0f, radius * 5.8f);
        popMatrix();

        stroke(rgba(40, 200, 120, 50));
        pushMatrix();
        translate(position.x, position.y);
        rotate(-angle * 1.5f + radians(55.0f));
        scale(1.0f, 0.3f);
        circle(0.0f, 0.0f, radius * 4.6f);
        popMatrix();

        // 6 crystalline spikes — alternating long and short
        stroke(rgba(70, 240, 155, 95));
        strokeWeight(1.0f);
        for (int i = 0; i < 6; ++i) {
            const float a = angle * 0.4f + i * radians(60.0f);
            const float r1 = radius * 1.3f;
            const float r2 = radius * (i % 2 == 0 ? 4.2f : 2.6f) * pulse;
            line(position.x + std::cos(a) * r1, position.y + std::sin(a) * r1, position.x + std::cos(a) * r2, position.y + std::sin(a) * r2);
        }
    }

    // ── Pulsar: ice blue — tight strobing core + two sweeping beams ───────────
    void showPulsar() const
    {
        // High-frequency strobe: abs(sin) gives double-pulse per cycle
        const float strobe = 0.55f + 0.45f * std::abs(std::sin(pulsePhase * 0.8f));

        noStroke();
        fill(rgba(25, 90, 220, static_cast<uint8_t>(12 * strobe)));
        circle(position.x, position.y, radius * 5.0f);
        fill(rgba(45, 130, 255, static_cast<uint8_t>(35 * strobe)));
        circle(position.x, position.y, radius * 3.0f);
        fill(rgba(90, 170, 255, static_cast<uint8_t>(100 * strobe)));
        circle(position.x, position.y, radius * 1.8f);
        fill(rgba(180, 220, 255, 210));
        circle(position.x, position.y, radius * 1.0f);
        fill(rgba(240, 248, 255, 255));
        circle(position.x, position.y, radius * 0.35f);

        // Two opposing beams: wide glow layer + sharp inner layer
        const float beamLen = radius * 5.5f * strobe;
        const float ca = std::cos(angle * 0.25f), sa = std::sin(angle * 0.25f);

        noFill();
        // Glow layer (wide, soft)
        stroke(rgba(60, 155, 255, static_cast<uint8_t>(45 * strobe)));
        strokeWeight(8.0f * strobe);
        line(position.x + ca * radius, position.y + sa * radius, position.x + ca * beamLen, position.y + sa * beamLen);
        line(position.x - ca * radius, position.y - sa * radius, position.x - ca * beamLen, position.y - sa * beamLen);
        // Sharp layer
        stroke(rgba(120, 200, 255, static_cast<uint8_t>(160 * strobe)));
        strokeWeight(1.8f * strobe);
        line(position.x + ca * radius, position.y + sa * radius, position.x + ca * beamLen, position.y + sa * beamLen);
        line(position.x - ca * radius, position.y - sa * radius, position.x - ca * beamLen, position.y - sa * beamLen);
    }

    void show() const
    {
        pushState();
        blendMode(BlendMode::additive);
        switch (type) {
            case StarType::Solar: showSolar(); break;
            case StarType::Nebula: showNebula(); break;
            case StarType::Pulsar: showPulsar(); break;
        }
        popState();
    }
};

// ─── PulseWave ────────────────────────────────────────────────────────────────
struct PulseWave
{
    float2 center;
    float t = 0.0f;
    float delay = 0.0f;
    float maxRadius = 0.0f;

    static constexpr float duration = 0.5f;

    bool alive() const { return delay > 0.0f || t < 1.0f; }

    void update(float dt)
    {
        if (delay > 0.0f) {
            delay -= dt;
            return;
        }
        t = std::min(t + dt / duration, 1.0f);
    }

    void show() const
    {
        if (delay > 0.0f || t >= 1.0f) return;

        // ease-out expansion: fast at first, decelerates at edge
        const float e = 1.0f - (1.0f - t) * (1.0f - t);
        const float r = e * maxRadius;
        const float fade = 1.0f - t;

        pushState();
        blendMode(BlendMode::additive);
        noFill();

        // Wide soft glow ring
        strokeWeight(10.0f * fade);
        stroke(rgba(220, 165, 80, static_cast<uint8_t>(fade * 55.0f)));
        circle(center.x, center.y, r * 2.0f);

        // Sharp inner ring
        strokeWeight(2.0f * fade + 0.3f);
        stroke(rgba(245, 195, 110, static_cast<uint8_t>(fade * 210.0f)));
        circle(center.x, center.y, r * 2.0f);

        popState();
    }
};

// ─── Gravitas ─────────────────────────────────────────────────────────────────
struct Gravitas : Sketch
{
    inline static constexpr float IMPULSE_COOLDOWN = 5.0f;

    Player player {20.0f};
    std::vector<Orbit> orbits;
    std::vector<Star> stars;
    std::vector<PulseWave> pulseWaves;
    std::vector<ScorePopup> scorePopups;
    std::vector<BackgroundStar> bgStars;
    float spawnTimer = 0.0f;
    uint32_t points = 0;
    float impulseTimer = 0.0f;

    void setup() override
    {
        setWindowSize(800, 600);
        setWindowResizable(false);

        // Seed background stars after window size is set
        for (int i = 0; i < 220; ++i) {
            bgStars.push_back({{random(static_cast<float>(width)), random(static_cast<float>(height))}, random(0.5f, 1.8f), random(100.0f)});
        }

        const float2 positions[3] = {
            {width * 0.25f, height * 0.25f},
            {width * 0.75f, height * 0.25f},
            {width * 0.5f, height * 0.75f},
        };
        const StarType types[3] = {StarType::Solar, StarType::Nebula, StarType::Pulsar};

        for (int i = 0; i < 3; ++i)
            stars.emplace_back(positions[i], 25.0f, types[i]);
    }

    void event(const WindowEvent& e) override
    {
        if (e.type != EventType::mousePress or impulseTimer > 0.0f) return;

        const float2 cursor {static_cast<float>(mouseX), static_cast<float>(mouseY)};

        for (Orbit& orbit : orbits) {
            const float2 toPlayer = cursor - orbit.position;
            const float dist = length(toPlayer);
            if (dist > PULL_RADIUS || dist < 1.0f) continue;

            const float newSpeed = std::max(length(orbit.velocity), PULL_STRENGTH);
            orbit.velocity = (toPlayer / dist) * newSpeed;
        }

        // Three staggered pulse rings
        for (int i = 0; i < 3; ++i)
            pulseWaves.push_back({cursor, 0.0f, i * 0.12f, PULL_RADIUS});
        impulseTimer = IMPULSE_COOLDOWN;
    }

    void draw() override
    {
        const float time = millis() / 1000.0f;

        spawnTimer += deltaTime;
        if (spawnTimer >= 1.0f) {
            orbits.push_back(spawnOrbit());
            spawnTimer = 0.0f;
        }

        impulseTimer = std::max(impulseTimer - deltaTime, 0.0f);

        // ── Base ──────────────────────────────────────────────────────────────
        background(8, 10, 18);

        // ── Background stars (additive) ───────────────────────────────────────
        pushState();
        blendMode(BlendMode::additive);
        for (const BackgroundStar& s : bgStars) s.show(time);
        popState();

        // ── Stars / targets ───────────────────────────────────────────────────
        for (Star& s : stars) {
            s.update(deltaTime);
            s.show();
        }

        // ── Pulse waves ───────────────────────────────────────────────────────
        for (PulseWave& w : pulseWaves) {
            w.update(deltaTime);
            w.show();
        }

        pulseWaves.erase(
            std::remove_if(pulseWaves.begin(), pulseWaves.end(), [](const PulseWave& w) {
                return !w.alive();
            }),
            pulseWaves.end()
        );

        // ── Orbits ────────────────────────────────────────────────────────────
        for (Orbit& o : orbits) {
            o.attract(player.position, deltaTime);
            o.update(deltaTime);
            o.show();
        }

        // ── Player ────────────────────────────────────────────────────────────
        player.update(deltaTime);
        player.show();

        // ── Scoring (capture position before erase) ───────────────────────────
        for (const Star& s : stars) {
            for (int j = static_cast<int>(orbits.size()) - 1; j >= 0; --j) {
                if (s.contains(orbits[j])) {
                    scorePopups.push_back({orbits[j].position, 0.0f, 10});
                    orbits.erase(orbits.begin() + j);
                    points += 10;
                }
            }
        }

        // ── Score popups ──────────────────────────────────────────────────────
        for (ScorePopup& p : scorePopups) {
            p.update(deltaTime);
            p.show();
        }

        scorePopups.erase(
            std::remove_if(scorePopups.begin(), scorePopups.end(), [](const ScorePopup& p) {
                return !p.alive();
            }),
            scorePopups.end()
        );

        // ── HUD (drawn last, always alpha blend, no glow contamination) ────────
        pushState();
        blendMode(BlendMode::alpha);
        noStroke();
        fill(rgba(180, 205, 255, 160));
        textAlign(HorizontalTextAlign::left, VerticalTextAlign::top);
        textSize(18.0f);
        text("ORBITS  " + std::to_string(orbits.size()), 16, 16);
        text("SCORE   " + std::to_string(points), 16, 40);
        popState();

        // -- Impulse cooldown overlay (drawn last, above HUD) ─────────────────────────
        pushState();
        {
            constexpr float PI = std::numbers::pi_v<float>;
            const float t = 1.0f - (impulseTimer / IMPULSE_COOLDOWN);
            const float progress = 0.5f - 0.5f * std::cos(t * PI);

            const float colorT = 0.4f + 0.6f * progress;
            const uint8_t a = static_cast<uint8_t>(colorT * 200.0f);
            const color_t onCooldownColor = rgba(255, 80, 60);
            const color_t readyColor = rgba(80, 255, 150);
            const color_t c = lerp(onCooldownColor, readyColor, progress);

            const auto easeInOutCubic = [](float x) {
                return x < 0.5f ? 4.0f * x * x * x : 1.0f - std::pow(-2.0f * x + 2.0f, 3) / 2.0f;
            };

            const float fillScale = easeInOutCubic(progress) * 0.9f + 0.1f;

            noFill();
            stroke(c);
            strokeWeight(3.0f);
            blendMode(BlendMode::alpha);
            translate(50.0f, height - 50.0f);
            // for (int i = 0; i < 4; ++i) {
            arc(0.0f, 0.0f, 60.0f, 60.0f, radians(45.0f) - radians(45.0f * progress), radians(90.0f * progress), ArcMode::open);
            arc(0.0f, 0.0f, 60.0f, 60.0f, radians(135.0f) - radians(45.0f * progress), radians(90.0f * progress), ArcMode::open);
            arc(0.0f, 0.0f, 60.0f, 60.0f, radians(225.0f) - radians(45.0f * progress), radians(90.0f * progress), ArcMode::open);
            arc(0.0f, 0.0f, 60.0f, 60.0f, radians(315.0f) - radians(45.0f * progress), radians(90.0f * progress), ArcMode::open);

            noStroke();
            fill(withAlpha(c, progress * 25.0f));
            pushMatrix();
            scale(fillScale, fillScale);
            circle(0.0f, 0.0f, 60.0f * fillScale);
            popMatrix();

            fill(rgba(255, 255, 255));
            textSize(18.0f);
            textAlign(HorizontalTextAlign::center, VerticalTextAlign::center);
            if (impulseTimer > 0.0f) {
                text(std::to_string(static_cast<int>(std::ceil(impulseTimer))), 0, 0);
            } else {
                text("R", 0, 0);
            }
        }
        popState();

        removeOffscreenOrbits();
    }

    void removeOffscreenOrbits()
    {
        orbits.erase(
            std::remove_if(orbits.begin(), orbits.end(), [](const Orbit& o) {
                return o.position.x < -SPAWN_MARGIN || o.position.x > width + SPAWN_MARGIN ||
                       o.position.y < -SPAWN_MARGIN || o.position.y > height + SPAWN_MARGIN;
            }),
            orbits.end()
        );
    }
};

namespace p5
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<Gravitas>();
    }
} // namespace p5
