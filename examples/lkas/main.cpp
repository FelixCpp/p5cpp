#include <p5cpp/p5cpp.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

using namespace p5cpp;

// ─── HSV → RGBA (h: 0–360, s/v: 0–1) ────────────────────────────────────────

static color_t hsv(float h, float s, float v, int a = 255)
{
    h = std::fmod(h, 360.f);
    const float c = v * s;
    const float x = c * (1.f - std::abs(std::fmod(h / 60.f, 2.f) - 1.f));
    const float m = v - c;
    float r, g, b;
    if      (h < 60)  { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else              { r = c; g = 0; b = x; }
    return rgba((int)((r + m) * 255), (int)((g + m) * 255), (int)((b + m) * 255), a);
}

// ─── Particle ─────────────────────────────────────────────────────────────────

struct Particle
{
    float2 pos;
    float2 vel;
    float  life;
    float  maxLife;
    float  hue;
    float  size;
};

static void spawnBurst(std::vector<Particle>& out, float wx, float wy, int count, float minSpeed, float maxSpeed, float baseHue)
{
    for (int i = 0; i < count; ++i)
    {
        const float angle = TWO_PI * i / count + randomFloat(-0.3f, 0.3f);
        const float life  = randomFloat(1.0f, 2.5f);
        out.push_back({
            .pos     = {wx, wy},
            .vel     = float2::fromAngle(angle) * randomFloat(minSpeed, maxSpeed),
            .life    = life,
            .maxLife = life,
            .hue     = baseHue + angle * (180.f / PI),
            .size    = randomFloat(3.f, 9.f),
        });
    }
}

// ─── Camera ───────────────────────────────────────────────────────────────────
//
// Transform:  screenPos = worldPos * zoom + offset
// Inverse:    worldPos  = (screenPos − offset) / zoom
//
// Zoom toward cursor: keep the world point under the mouse fixed.
//   newOffset = mouse − (mouse − offset) * (newZoom / zoom)

class CameraController2D
{
public:
    float2 offset = {0.f, 0.f};
    float  zoom   = 1.f;

    float2 screenToWorld(float sx, float sy) const
    {
        return {(sx - offset.x) / zoom, (sy - offset.y) / zoom};
    }

    void handleEvent(const WindowEvent& e)
    {
        if (e.type == EventType::mouseScroll)
        {
            const float mx      = (float)e.mouseScroll.x;
            const float my      = (float)e.mouseScroll.y;
            const float factor  = (e.mouseScroll.dy > 0.f) ? 1.12f : 1.f / 1.12f;
            const float newZoom = std::clamp(zoom * factor, 0.05f, 40.f);
            // Anchor the world point currently under the mouse
            const float ratio = newZoom / zoom;
            offset.x = mx - (mx - offset.x) * ratio;
            offset.y = my - (my - offset.y) * ratio;
            zoom     = newZoom;
        }

        if (e.type == EventType::mousePress   && e.mouseButton.button == MouseButton::right) m_dragging = true;
        if (e.type == EventType::mouseRelease && e.mouseButton.button == MouseButton::right) m_dragging = false;
    }

    // Call once per frame
    void update()
    {
        if (m_dragging)
        {
            offset.x += (float)(getMouseX() - getPMouseX());
            offset.y += (float)(getMouseY() - getPMouseY());
        }
    }

    // Adaptive grid drawn in screen space — always 1 px, no zoom scaling
    void drawGrid() const
    {
        const float sw = (float)getLogicalWidth();
        const float sh = (float)getLogicalHeight();

        // Pick a world-space step so lines appear ~80 px apart on screen
        const float rawStep = 80.f / zoom;
        const float mag     = std::pow(10.f, std::floor(std::log10(rawStep)));
        float worldStep     = mag;
        if      (rawStep / mag >= 5.f) worldStep = mag * 5.f;
        else if (rawStep / mag >= 2.f) worldStep = mag * 2.f;

        const float screenStep = worldStep * zoom;

        noFill();
        stroke(70, 75, 110, 30);
        strokeWeight(1.f);

        float sx0 = std::fmod(offset.x, screenStep);
        if (sx0 < 0.f) sx0 += screenStep;
        for (float sx = sx0; sx <= sw; sx += screenStep)
            line(sx, 0.f, sx, sh);

        float sy0 = std::fmod(offset.y, screenStep);
        if (sy0 < 0.f) sy0 += screenStep;
        for (float sy = sy0; sy <= sh; sy += screenStep)
            line(0.f, sy, sw, sy);

        // Brighter lines through world origin
        stroke(100, 105, 160, 65);
        if (offset.x >= 0.f && offset.x <= sw) line(offset.x, 0.f, offset.x, sh);
        if (offset.y >= 0.f && offset.y <= sh) line(0.f, offset.y, sw, offset.y);
    }

    template <typename Fn>
    void applyTransform(Fn&& fn) const
    {
        pushMatrix();
        translate(offset.x, offset.y);
        scale(zoom, zoom);
        fn();
        popMatrix();
    }

private:
    bool m_dragging = false;
};

// ─── Sketch ───────────────────────────────────────────────────────────────────

struct GalaxySketch : Sketch
{
    std::vector<Particle> particles;
    CameraController2D    camera;
    float                 hueOffset = 0.f;

    void setup() override
    {
        setWindowSize(900, 700);
        setWindowTitle("Galaxy Vortex  |  Mouse=emit  Right-drag=pan  Scroll=zoom  Click=explode  ESC=quit");
        frameRate(60);
        particles.reserve(8000);
    }

    void draw() override
    {
        background(4, 4, 12, 18); // low-alpha clear → motion-blur trail

        camera.update();
        camera.drawGrid(); // screen-space, always crisp on top of the fade

        const float  dt         = getDeltaTime();
        const float2 worldMouse = camera.screenToWorld((float)getMouseX(), (float)getMouseY());
        hueOffset += dt * 50.f;

        camera.applyTransform([&]()
        {
            // Continuous vortex emitter at the world-space cursor position
            for (int i = 0; i < 10; ++i)
            {
                const float angle = randomFloat(0.f, TWO_PI);
                const float speed = randomFloat(100.f, 320.f);
                const float life  = randomFloat(1.2f, 3.0f);
                float2      dir   = float2::fromAngle(angle);
                float2      vel   = dir * speed + dir.perpendicular() * randomFloat(-80.f, 80.f);
                vel.y -= randomFloat(10.f, 40.f); // slight upward bias
                particles.push_back({
                    .pos     = worldMouse + dir * randomFloat(0.f, 20.f),
                    .vel     = vel,
                    .life    = life,
                    .maxLife = life,
                    .hue     = hueOffset + angle * (180.f / PI),
                    .size    = randomFloat(5.f, 15.f),
                });
            }

            blendMode(BlendMode::additive); // overlapping particles bloom and glow
            noStroke();

            for (auto& p : particles)
            {
                p.vel.y += 120.f * dt; // gravity
                p.vel   *= 0.993f;     // air drag
                p.pos   += p.vel * dt;
                p.life  -= dt;

                const float t = p.life / p.maxLife;
                fill(hsv(p.hue, 1.f, 1.f, (int)(t * t * 210.f)));
                circle(p.pos.x, p.pos.y, p.size * t);
            }

            blendMode(BlendMode::alpha);
        });

        std::erase_if(particles, [](const Particle& p) { return p.life <= 0.f; });
    }

    void event(const WindowEvent& e) override
    {
        camera.handleEvent(e);

        if (e.type == EventType::mousePress && e.mouseButton.button == MouseButton::left)
        {
            const float2 w = camera.screenToWorld((float)e.mouseButton.x, (float)e.mouseButton.y);
            spawnBurst(particles, w.x, w.y, 160, 150.f, 600.f, hueOffset);
        }

        if (e.type == EventType::keyPress && e.keyEvent.key == Key::escape)
            quit();
    }
};

std::unique_ptr<Sketch> p5cpp::createSketch()
{
    return std::make_unique<GalaxySketch>();
}
