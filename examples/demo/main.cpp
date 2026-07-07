// flowfield_sketch.cpp
// -----------------------------------------------------------------------
// "Wow"-Demo für die p5cpp-README: ein Flow-Field aus farbigen Partikeln,
// die entlang eines animierten Perlin-Noise-Feldes strömen. Durch den
// Trail-Effekt (kein hartes background(), sondern ein halbtransparentes
// Rechteck) entstehen weiche, organische Linienbahnen, die sich über die
// Zeit langsam verändern -> ideal für einen kurzen GIF-Loop.
//
// ANNAHMEN (bitte prüfen, da nicht im gezeigten Header sichtbar):
//   - p5cpp::noise(float x, float y, float z) existiert (p5.js-Konvention,
//     deklariert vermutlich in math/noise.hpp)
//   - Es gibt KEINE Abhängigkeit von color_t-Konstruktoren; es werden nur
//     die im Header sichtbaren fill(int,int,int,int)-Overloads genutzt.
//   - Für Zufallszahlen wird bewusst std::rand() genutzt statt einer
//     evtl. vorhandenen p5cpp::random()-Funktion, um keine falsche
//     Signatur zu raten. Kannst du 1:1 gegen p5cpp::random(min, max)
//     tauschen, falls vorhanden.
// -----------------------------------------------------------------------

#include <p5cpp/p5cpp.hpp>

#include <cmath>
#include <cstdlib>
#include <vector>

using namespace p5cpp;

namespace
{
    struct Particle
    {
        float x, y;   // aktuelle Position
        float px, py; // vorherige Position (für die Linie)
        float hue;    // Farbphase 0..1
    };

    constexpr int PARTICLE_COUNT = 900;
    constexpr float NOISE_SCALE = 0.0035f;
    constexpr float TIME_SCALE = 0.12f;
    constexpr float SPEED = 2.4f;
    constexpr float PI_F = 3.14159265358979f;

    bool started = false;
    std::vector<Particle> particles;

    float randRange(float lo, float hi)
    {
        return lo + (hi - lo) * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX));
    }

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
} // namespace

void setup()
{
    setWindowSize(800, 800);
    setWindowTitle("p5cpp - Flow Field");
    setWindowResizable(false);
    frameRate(60);

    particles.reserve(PARTICLE_COUNT);
    for (int i = 0; i < PARTICLE_COUNT; ++i) {
        float x = randRange(0.0f, static_cast<float>(getLogicalWidth()));
        float y = randRange(0.0f, static_cast<float>(getLogicalHeight()));
        particles.push_back(Particle {x, y, x, y, randRange(0.0f, 1.0f)});
    }

    noStroke();
    fill(8, 10, 20, 255);
    rect(0, 0, static_cast<float>(getLogicalWidth()), static_cast<float>(getLogicalHeight()));
}

void draw()
{
    // Statt hartem Clear: ein fast transparentes Rechteck über den ganzen
    // Screen -> erzeugt die weichen, ausblassenden Trails.
    noStroke();
    fill(8, 10, 20, 50);
    rect(0, 0, static_cast<float>(getLogicalWidth()), static_cast<float>(getLogicalHeight()));

    const float t = getGlobalTime();
    const float w = static_cast<float>(getLogicalWidth());
    const float h = static_cast<float>(getLogicalHeight());

    for (auto& p : particles) {
        p.px = p.x;
        p.py = p.y;

        // Winkel aus dem Noise-Feld -> ergibt sanfte, wirbelnde Strömungen
        float n = noise(p.x * NOISE_SCALE, p.y * NOISE_SCALE, t * TIME_SCALE);
        float angle = n * PI_F * 4.0f;

        if (started) {
            p.x += std::cos(angle) * SPEED;
            p.y += std::sin(angle) * SPEED;
        }

        // Weiches Wraparound an den Rändern
        if (p.x < 0.0f) {
            p.x += w;
            p.px = p.x;
        }
        if (p.x > w) {
            p.x -= w;
            p.px = p.x;
        }
        if (p.y < 0.0f) {
            p.y += h;
            p.py = p.y;
        }
        if (p.y > h) {
            p.y -= h;
            p.py = p.y;
        }

        p.hue = std::fmod(p.hue + 0.0009f, 1.0f);

        int r, g, b;
        hsbToRgb(p.hue, 0.75f, 1.0f, r, g, b);

        stroke(r, g, b, 220);
        strokeWeight(1.6f);
        line(p.px, p.py, p.x, p.y);
    }
}

class DemoSketch : public p5cpp::Sketch
{

    void setup() override
    {
        ::setup();
    }

    void event(const p5cpp::WindowEvent& e) override
    {
        if (e.type == p5cpp::EventType::mousePress) {
            started = not started;
        }
    }

    void draw() override
    {
        ::draw();
    }
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<DemoSketch>();
    }
} // namespace p5cpp
