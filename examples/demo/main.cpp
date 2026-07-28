// gravity_playground.cpp
// -----------------------------------------------------------------------
// Interaktive 2D-Szene UND lebendes Beispiel für p5cpp's Pre-/Post-Modul-
// Mechanismus (Sketch::registerModules(ModuleRegistrar&)).
//
// Der eigentliche Sketch (GravityPlaygroundSketch) kennt nur Partikel,
// Maus und Klicks. Drei zusätzliche Module hängen sich unabhängig davon
// vor bzw. nach den Sketch-Aufruf:
//
//   - BackdropModule  (BEFORE) räumt vor jedem draw() den Screen frei
//                      (Trail-Fade) — das MUSS vor dem Sketch laufen,
//                      sonst würde es die frisch gezeichneten Partikel
//                      wieder überdecken. Weil es aber ein "Before"-Modul
//                      ist, kann es via next() trotzdem NACH allem
//                      anderen (Sketch, Vignette, HUD) noch den
//                      pulsierenden Rahmen ganz oben drüberzeichnen —
//                      Onion-Semantik in Aktion.
//   - VignetteModule  (AFTER)  legt eine Vignette über das fertige Bild.
//   - HudModule       (AFTER)  zeigt Live-Status der beiden anderen
//                      Module — per AppContext-Service-Lookup, exakt wie
//                      im README-Beispiel für TimerModule.
//
// Jedes der drei Module toggelt sich über sein eigenes event() komplett
// unabhängig vom Sketch (Tasten 1/2/3) — man sieht live, wie sich die
// Optik ändert, ohne dass GravityPlaygroundSketch davon auch nur weiß.
//
// Steuerung:
//   Maus bewegen   - Gravitationszentrum folgt dem Cursor
//   Linksklick     - Partikel-Burst am Klickpunkt
//   Rechtsklick    - kurzlebiger Abstoßer (Repulsor) am Klickpunkt
//   1 / 2 / 3      - Trails (pre) / Vignette (post) / HUD (post) toggeln
//   Escape         - beenden
// -----------------------------------------------------------------------

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/application/app_context.hpp>

#include <algorithm>
#include <cmath>
#include <format>
#include <vector>

using namespace p5cpp;

namespace
{
    constexpr int PARTICLE_COUNT = 1400;
    constexpr float ATTRACTION = 340.0f;
    constexpr float SWIRL = 260.0f;
    constexpr float DRAG = 0.985f;
    constexpr float MAX_SPEED = 420.0f;
    constexpr float RESPAWN_RADIUS = 520.0f;

    constexpr int BURST_SIZE = 140;
    constexpr float BURST_SPEED_MIN = 250.0f;
    constexpr float BURST_SPEED_MAX = 750.0f;

    constexpr float REPULSOR_LIFETIME = 2.4f;
    constexpr float REPULSOR_STRENGTH = 42000000.0f;

    // Einfache HSB(0..1) -> RGB(0..255) Konvertierung, unabhängig von
    // p5cpp's color_t, damit wir nur die bekannten fill(r,g,b,a)-Overloads
    // brauchen.
    void hsbToRgb(float h, float s, float v, int& r, int& g, int& b)
    {
        h = std::fmod(h, 1.0f) * 6.0f;
        int i = static_cast<int>(h);
        float f = h - static_cast<float>(i);
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));

        float rf, gf, bf;
        switch (i % 6) {
            case 0:
                rf = v;
                gf = t;
                bf = p;
                break;
            case 1:
                rf = q;
                gf = v;
                bf = p;
                break;
            case 2:
                rf = p;
                gf = v;
                bf = t;
                break;
            case 3:
                rf = p;
                gf = q;
                bf = v;
                break;
            case 4:
                rf = t;
                gf = p;
                bf = v;
                break;
            default:
                rf = v;
                gf = p;
                bf = q;
                break;
        }
        r = static_cast<int>(rf * 255.0f);
        g = static_cast<int>(gf * 255.0f);
        b = static_cast<int>(bf * 255.0f);
    }

    struct Particle
    {
        float2 pos;
        float2 vel;
        float hue;
        float size;
    };

    struct Repulsor
    {
        float2 pos;
        float life;
        float maxLife;
    };

    Particle makeParticle(float2 pos)
    {
        return Particle {
            .pos = pos,
            .vel = float2::fromAngle(randomFloat(0.0f, TWO_PI)) * randomFloat(20.0f, 90.0f),
            .hue = randomFloat(0.0f, 1.0f),
            .size = randomFloat(2.5f, 5.5f),
        };
    }
} // namespace

// -----------------------------------------------------------------------
// BEFORE-Modul: räumt den Screen vor dem Sketch frei (Trail-Fade) und
// zeichnet - dank next() - einen Rahmen, der über wirklich allem liegt.
// -----------------------------------------------------------------------
class BackdropModule : public Module
{
public:
    void setup(AppContext& context, Next next) override
    {
        context.registerService<BackdropModule>(this);
        background(6, 8, 16, 255);
        next();
    }

    void event(AppContext& context, WindowEvent& event, Next next) override
    {
        if (event.type == EventType::keyPress && event.keyEvent.key == Key::n1) {
            m_trailsEnabled = not m_trailsEnabled;
        }
        next();
    }

    void draw(AppContext& context, Next next) override
    {
        noStroke();
        background(6, 8, 16, m_trailsEnabled ? 40 : 255);

        next(); // Sketch, Vignette und HUD zeichnen jetzt ihren gesamten Inhalt.

        // Alles, was HIER steht, landet trotzdem ganz oben im Bild - obwohl
        // dieses Modul VOR dem Sketch registriert wurde. So mächtig ist die
        // Onion-Kette: Position in der Registrierung != Position im Bild.
        const float w = static_cast<float>(getLogicalWidth());
        const float h = static_cast<float>(getLogicalHeight());

        int r, g, b;
        hsbToRgb(std::fmod(getGlobalTime() * 0.05f, 1.0f), 0.55f, 1.0f, r, g, b);

        noFill();
        stroke(r, g, b, 200);
        strokeWeight(4.0f);
        rect(2.0f, 2.0f, w - 4.0f, h - 4.0f);
    }

    void destroy(AppContext& context, Next next) override
    {
        context.unregisterService<BackdropModule>();
        next();
    }

    bool isTrailsEnabled() const { return m_trailsEnabled; }

private:
    bool m_trailsEnabled = true;
};

// -----------------------------------------------------------------------
// AFTER-Modul: legt eine Vignette über das fertige Bild des Sketches.
// -----------------------------------------------------------------------
class VignetteModule : public Module
{
public:
    void setup(AppContext& context, Next next) override
    {
        context.registerService<VignetteModule>(this);
        next();
    }

    void event(AppContext& context, WindowEvent& event, Next next) override
    {
        if (event.type == EventType::keyPress && event.keyEvent.key == Key::n2) {
            m_enabled = not m_enabled;
        }
        next();
    }

    void draw(AppContext& context, Next next) override
    {
        if (m_enabled) {
            const float w = static_cast<float>(getLogicalWidth());
            const float h = static_cast<float>(getLogicalHeight());
            const float radius = std::max(w, h) * 0.8f;

            blendMode(BlendMode::alpha);
            noStroke();
            fill(0, 0, 0, 170);
            circle(0.0f, 0.0f, radius);
            circle(w, 0.0f, radius);
            circle(0.0f, h, radius);
            circle(w, h, radius);
        }

        next();
    }

    void destroy(AppContext& context, Next next) override
    {
        context.unregisterService<VignetteModule>();
        next();
    }

    bool isEnabled() const { return m_enabled; }

private:
    bool m_enabled = true;
};

// -----------------------------------------------------------------------
// AFTER-Modul: zeigt Live-Status der anderen Module per AppContext-Lookup
// - genau das "require<T>() von jeder API-Funktion aus"-Pattern, das das
// README für Custom Engine Modules beschreibt.
// -----------------------------------------------------------------------
class HudModule : public Module
{
public:
    void event(AppContext& context, WindowEvent& event, Next next) override
    {
        if (event.type == EventType::keyPress && event.keyEvent.key == Key::n3) {
            m_visible = not m_visible;
        }
        next();
    }

    void draw(AppContext& context, Next next) override
    {
        if (m_visible) {
            const BackdropModule& backdrop = context.require<BackdropModule>();
            const VignetteModule& vignette = context.require<VignetteModule>();

            blendMode(BlendMode::alpha);
            noStroke();
            fill(0, 0, 0, 150);
            rect(16.0f, 16.0f, 380.0f, 158.0f, BorderRadius::circular(10.0f));

            fill(255);
            textAlign(TextAlign::topLeft);

            textSize(18.0f);
            text("p5cpp - Gravity Playground", 30.0f, 30.0f);

            textSize(14.0f);
            text(std::format("FPS: {}   Particles: {}", getFrameRate(), PARTICLE_COUNT), 30.0f, 58.0f);
            text(std::format("[1] Trails   (pre-module,  BackdropModule): {}", backdrop.isTrailsEnabled() ? "ON" : "OFF"), 30.0f, 80.0f);
            text(std::format("[2] Vignette (post-module, VignetteModule): {}", vignette.isEnabled() ? "ON" : "OFF"), 30.0f, 100.0f);
            text("[3] This HUD (post-module, HudModule)", 30.0f, 120.0f);
            text("Move: gravity | Click: burst | Right-click: repel", 30.0f, 148.0f);
        }

        next();
    }

private:
    bool m_visible = true;
};

// -----------------------------------------------------------------------
// Der eigentliche Sketch - kennt nur Partikelphysik und Eingaben, weiß
// nichts von Trails, Vignette oder HUD. Die drei Module oben werden
// ausschließlich über registerModules() angeflanscht.
// -----------------------------------------------------------------------
class GravityPlaygroundSketch : public Sketch
{
public:
    void registerModules(ModuleRegistrar& registrar) override
    {
        registrar.addModuleBefore(std::make_unique<BackdropModule>());
        registrar.addModuleAfter(std::make_unique<VignetteModule>());
        registrar.addModuleAfter(std::make_unique<HudModule>());
    }

    void setup() override
    {
        setWindowSize(960, 720);
        setWindowTitle("p5cpp - Gravity Playground");
        setWindowResizable(false);
        frameRate(60);

        particles.reserve(PARTICLE_COUNT);
        for (int i = 0; i < PARTICLE_COUNT; ++i) {
            const float2 pos {randomFloat(0.0f, static_cast<float>(getLogicalWidth())), randomFloat(0.0f, static_cast<float>(getLogicalHeight()))};
            particles.push_back(makeParticle(pos));
        }
    }

    void event(const WindowEvent& e) override
    {
        if (e.type == EventType::mousePress && e.mouseButton.button == MouseButton::left) {
            burst(float2 {static_cast<float>(e.mouseButton.x), static_cast<float>(e.mouseButton.y)});
        }

        if (e.type == EventType::mousePress && e.mouseButton.button == MouseButton::right) {
            const float2 pos {static_cast<float>(e.mouseButton.x), static_cast<float>(e.mouseButton.y)};
            repulsors.push_back({.pos = pos, .life = REPULSOR_LIFETIME, .maxLife = REPULSOR_LIFETIME});
        }

        if (e.type == EventType::keyPress && e.keyEvent.key == Key::escape) {
            quit();
        }
    }

    void draw() override
    {
        const float dt = getDeltaTime();
        const float2 gravityCenter {static_cast<float>(getMouseX()), static_cast<float>(getMouseY())};

        for (Repulsor& r : repulsors) {
            r.life -= dt;
        }
        std::erase_if(repulsors, [](const Repulsor& r) {
            return r.life <= 0.0f;
        });

        blendMode(BlendMode::additive);
        noStroke();

        for (Particle& p : particles) {
            const float2 toCenter = gravityCenter - p.pos;
            const float distance = length(toCenter);
            const float2 direction = distance > 0.0001f ? toCenter / distance : float2::zero;
            const float2 tangent = perpendicular(direction);

            float2 accel = direction * ATTRACTION + tangent * SWIRL;

            for (const Repulsor& r : repulsors) {
                const float2 away = p.pos - r.pos;
                const float distSq = std::max(lengthSquared(away), 250.0f);
                const float t = r.life / r.maxLife;
                accel += normalized(away) * (REPULSOR_STRENGTH * t / distSq);
            }

            p.vel += accel * dt;
            p.vel *= DRAG;
            p.vel = limited(p.vel, MAX_SPEED);
            p.pos += p.vel * dt;

            if (length(p.pos - gravityCenter) > RESPAWN_RADIUS) {
                p = makeParticle(gravityCenter + float2::fromAngle(randomFloat(0.0f, TWO_PI)) * randomFloat(20.0f, 80.0f));
            }

            p.hue = std::fmod(p.hue + dt * 0.05f, 1.0f);

            int r, g, b;
            hsbToRgb(p.hue, 0.8f, 1.0f, r, g, b);
            const float speedT = std::min(length(p.vel) / MAX_SPEED, 1.0f);
            fill(r, g, b, static_cast<int>(140.0f + speedT * 100.0f));
            circle(p.pos.x, p.pos.y, p.size);
        }

        blendMode(BlendMode::alpha);

        noFill();
        for (const Repulsor& r : repulsors) {
            const float t = r.life / r.maxLife;
            stroke(255, 90, 90, static_cast<int>(200.0f * t));
            strokeWeight(3.0f);
            circle(r.pos.x, r.pos.y, 40.0f + (1.0f - t) * 60.0f);
        }

        stroke(255, 255, 255, 160);
        strokeWeight(2.0f);
        circle(gravityCenter.x, gravityCenter.y, 14.0f + std::sin(getGlobalTime() * 4.0f) * 4.0f);
    }

private:
    void burst(float2 pos)
    {
        for (int i = 0; i < BURST_SIZE; ++i) {
            Particle& p = particles[static_cast<size_t>(randomInt(0, static_cast<int>(particles.size()) - 1))];
            p.pos = pos;
            p.vel = float2::fromAngle(randomFloat(0.0f, TWO_PI)) * randomFloat(BURST_SPEED_MIN, BURST_SPEED_MAX);
            p.hue = randomFloat(0.0f, 1.0f);
        }
    }

    std::vector<Particle> particles;
    std::vector<Repulsor> repulsors;
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<GravityPlaygroundSketch>();
    }
} // namespace p5cpp
