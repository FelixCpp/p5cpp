#include <p5.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <memory>
#include <numbers>
#include <random>
#include <string>
#include <vector>

using namespace p5;

static constexpr float PI = std::numbers::pi_v<float>;
static constexpr float TAU = 2.f * PI;

// =============================================================================
//  GRAVITAS
//
//  You are a star.  Move with the mouse – your gravity pulls drifting cosmic
//  "seeds" through space.  Guide them into the glowing portals before they
//  time out or fall into a Void Rift.
//
//  Controls:
//    Mouse       – move your star (gravity follows)
//    Left Click  – Gravitational Pulse (ring that boosts nearby seeds toward you)
//    ESC         – return to menu
//
//  Visual highlights:
//    • GLSL deep-space background with nebula wisps + distant galaxies
//    • 220 twinkling background stars
//    • Three coloured portals with rotating inner sigils
//    • Void Rifts: slowly drifting spiral dark-matter vortexes
//    • Seeds: glowing orbs with coloured trails that respond to gravity
//    • Player star: solar corona with pulsing rays
//    • No shooting – pure gravitational puzzle
// =============================================================================

struct GravitasSketch : p5::Sketch
{
    // ── Layout ────────────────────────────────────────────────────────────────
    static constexpr int W = 800;
    static constexpr int H = 800;
    static constexpr float G_CONST = 24000.f; // gravitational constant
    static constexpr float SEED_LIFE = 21.f;
    static constexpr float PORTAL_R = 38.f;
    static constexpr float VOID_PULL = 5500.f;
    static constexpr float PULSE_CD = 4.0f;
    static constexpr float PULSE_MAX_R = 250.f;
    static constexpr float PULSE_SPD = 310.f;
    static constexpr float STAR_FOLLOW = 5.5f;

    // ── State ─────────────────────────────────────────────────────────────────
    enum class State { Menu,
                       Playing,
                       GameOver };
    State state = State::Menu;

    // ── Player star ───────────────────────────────────────────────────────────
    float starX = W * 0.5f;
    float starY = H * 0.5f;
    float starMass = 1.0f;
    float starPhase = 0.f;
    float pulseCd = 0.f;
    float pulseR = -1.f; // -1 = inactive

    // ── Seeds ─────────────────────────────────────────────────────────────────
    struct Seed
    {
        float x, y, vx, vy;
        float life, maxLife;
        float hue;
        float size;
        float wobble = 0.f;
        bool alive = true;

        // circular trail buffer
        std::array<float2, 36> trail {};
        int trailHead = 0, trailCount = 0;
        float trailTimer = 0.f;
    };
    std::vector<Seed> seeds;

    // ── Portals ───────────────────────────────────────────────────────────────
    struct Portal
    {
        float x, y;
        float angle = 0.f;
        float pulseT = 0.f; // flash on collection (0..1)
        float hue;
    };
    std::array<Portal, 3> portals;

    // ── Void Rifts ────────────────────────────────────────────────────────────
    struct VoidRift
    {
        float x, y, vx, vy;
        float life, maxLife;
        float angle = 0.f;
        float size;
        bool alive = true;
    };
    std::vector<VoidRift> voidRifts;

    // ── Effects ───────────────────────────────────────────────────────────────
    struct Particle
    {
        float x, y, vx, vy, life, maxLife, size;
        color_t col;
    };
    struct Ring
    {
        float x, y, startR, endR, life, maxLife;
        color_t col;
        float curR() const { return startR + (endR - startR) * (1.f - life / maxLife); }
    };
    struct FloatText
    {
        float x, y, vy, life, maxLife;
        std::string text;
        color_t col;
    };

    std::vector<Particle> particles;
    std::vector<Ring> rings;
    std::vector<FloatText> floatTexts;

    // ── Background stars ──────────────────────────────────────────────────────
    struct BgStar
    {
        float x, y, brightness, phase, size;
    };
    std::array<BgStar, 220> bgStars;

    // ── Progression ───────────────────────────────────────────────────────────
    int score = 0;
    int hiScore = 0;
    int combo = 0;
    float comboTimer = 0.f;
    int lives = 5;

    float seedTimer = 0.f;
    float seedIv = 2.8f;
    float voidTimer = 16.f;

    // ── Timing ────────────────────────────────────────────────────────────────
    float bgTime = 0.f;
    float shakeX = 0.f, shakeY = 0.f, shakeDur = 0.f;

    // ── Rendering ─────────────────────────────────────────────────────────────
    std::shared_ptr<Shader> bgShader;
    std::shared_ptr<Framebuffer> bgCanvas;

    // ── RNG ───────────────────────────────────────────────────────────────────
    std::mt19937 rng {std::random_device {}()};
    float fr(float lo, float hi) { return std::uniform_real_distribution<float> {lo, hi}(rng); }

    // Convert hue (0..360), saturation, value → color_t
    color_t hsv(float h, float s, float v, float a = 1.f)
    {
        h = std::fmod(h, 360.f);
        if (h < 0.f) h += 360.f;
        float c = v * s;
        float x = c * (1.f - std::abs(std::fmod(h / 60.f, 2.f) - 1.f));
        float m = v - c;
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
        return rgba((int)((r + m) * 255), (int)((g + m) * 255), (int)((b + m) * 255), (int)(a * 255));
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Lifecycle
    // ─────────────────────────────────────────────────────────────────────────
    void setup() override
    {
        setWindowSize(W, H);
        setWindowTitle("GRAVITAS");
        setWindowResizable(false);
        frameRate(60);
        buildBgShader();
        bgCanvas = std::shared_ptr<Framebuffer>(createFramebuffer(W, H).release());

        for (auto& s : bgStars) {
            s.x = fr(0.f, W);
            s.y = fr(0.f, H);
            s.brightness = fr(0.1f, 1.0f);
            s.phase = fr(0.f, TAU);
            s.size = fr(0.5f, 2.8f);
        }

        // Three portals: upper-left, upper-right, lower-center
        portals[0] = {175.f, 175.f, 0.f, 0.f, 165.f}; // cyan-green
        portals[1] = {625.f, 175.f, 0.f, 0.f, 270.f}; // violet-purple
        portals[2] = {400.f, 635.f, 0.f, 0.f, 45.f};  // amber
    }

    void destroy() override {}

    // ─────────────────────────────────────────────────────────────────────────
    // GLSL background: deep space with nebula + galaxies
    // ─────────────────────────────────────────────────────────────────────────
    void buildBgShader()
    {
        static constexpr const char* VERT = R"glsl(
            #version 410 core
            layout(location=0) in vec2  a_Position;
            layout(location=1) in vec2  a_TexCoord;
            layout(location=2) in vec4  a_Color;
            layout(location=3) in float a_TexIndex;
            out vec2 v_TexCoord; out vec4 v_Color; out float v_TexIndex;
            uniform mat4 u_ProjectionMatrix;
            void main() {
                gl_Position = u_ProjectionMatrix * vec4(a_Position, 0.0, 1.0);
                v_TexCoord  = a_TexCoord;
                v_Color     = a_Color;
                v_TexIndex  = a_TexIndex;
            }
        )glsl";

        static constexpr const char* FRAG = R"glsl(
            #version 410 core
            out vec4 o_Color;
            uniform float uTime;
            uniform vec2  uRes;

            float hash(vec2 p)  { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5); }
            float hash1(float p){ return fract(sin(p * 127.1) * 43758.5); }
            float noise(vec2 p) {
                vec2 i = floor(p), f = p - i;
                f = f * f * (3.0 - 2.0 * f);
                return mix(mix(hash(i), hash(i + vec2(1,0)), f.x),
                           mix(hash(i + vec2(0,1)), hash(i + vec2(1,1)), f.x), f.y);
            }

            void main() {
                vec2 uv = gl_FragCoord.xy / uRes;

                // Near-black space
                vec3 col = vec3(0.008, 0.004, 0.020);

                // Nebula wisps (4 overlapping FBM clouds)
                for (int i = 0; i < 4; i++) {
                    float fi  = float(i);
                    vec2  nuv = uv * 2.2 + vec2(fi * 1.9, fi * 2.6);
                    nuv += 0.07 * vec2(sin(uTime * 0.035 + fi), cos(uTime * 0.028 + fi));
                    float n = noise(nuv) * noise(nuv * 1.9 + 4.1);
                    vec3  nc = vec3(
                        0.15 + 0.25 * sin(fi * 2.3),
                        0.04 + 0.08 * sin(fi * 1.4 + 1.6),
                        0.35 + 0.30 * cos(fi * 1.8 + 0.5)
                    );
                    col += nc * n * 0.09;
                }

                // Distant galaxy 1 (upper-left)
                vec2  g1c = uv - vec2(0.18, 0.22);
                float g1d = length(g1c);
                float g1a = atan(g1c.y, g1c.x) + g1d * 9.0 + uTime * 0.018;
                col += vec3(0.45, 0.25, 0.65) * exp(-g1d * 5.5)
                       * (0.5 + 0.5 * sin(g1a * 3.0 + g1d * 18.0)) * 0.045;

                // Distant galaxy 2 (lower-right)
                vec2  g2c = uv - vec2(0.80, 0.76);
                float g2d = length(g2c);
                float g2a = atan(g2c.y, g2c.x) + g2d * 11.0 - uTime * 0.013;
                col += vec3(0.25, 0.45, 0.75) * exp(-g2d * 7.0)
                       * (0.5 + 0.5 * sin(g2a * 4.0 + g2d * 22.0)) * 0.038;

                // Vignette
                vec2 cv = uv - 0.5;
                col *= 1.0 - dot(cv * 1.5, cv * 1.5);

                o_Color = vec4(col, 1.0);
            }
        )glsl";

        bgShader = std::shared_ptr<Shader>(loadShader(VERT, FRAG).release());
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Events
    // ─────────────────────────────────────────────────────────────────────────
    void event(const WindowEvent& e) override
    {
        if (state == State::Menu || state == State::GameOver) {
            if ((e.type == EventType::keyPress && e.keyEvent.key == Key::enter) ||
                e.type == EventType::mousePress)
                startGame();
            return;
        }
        if (e.type == EventType::keyPress && e.keyEvent.key == Key::escape) {
            state = State::Menu;
            return;
        }
        if (e.type == EventType::mousePress && e.mouseButton.button == MouseButton::left)
            firePulse();
    }

    void startGame()
    {
        state = State::Playing;
        score = 0;
        combo = 0;
        comboTimer = 0.f;
        lives = 5;
        seedTimer = 0.f;
        seedIv = 2.8f;
        voidTimer = 16.f;
        pulseCd = 0.f;
        pulseR = -1.f;
        starMass = 1.0f;
        starX = W * 0.5f;
        starY = H * 0.5f;
        shakeDur = 0.f;
        seeds.clear();
        voidRifts.clear();
        particles.clear();
        rings.clear();
        floatTexts.clear();
    }

    void firePulse()
    {
        if (pulseCd > 0.f) return;
        pulseCd = PULSE_CD;
        pulseR = 0.f;
        rings.push_back({starX, starY, 0.f, PULSE_MAX_R, 0.55f, 0.55f, rgba(255, 215, 80)});
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Main draw
    // ─────────────────────────────────────────────────────────────────────────
    void draw() override
    {
        bgTime += deltaTime;
        renderBg();

        if (state == State::Menu) {
            drawBgStars();
            drawMenu();
            return;
        }
        if (state == State::GameOver) {
            drawBgStars();
            drawScene(false);
            drawGameOver();
            return;
        }

        float dt = std::min(deltaTime, 0.05f);
        update(dt);
        drawBgStars();
        drawScene(true);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Update
    // ─────────────────────────────────────────────────────────────────────────
    void update(float dt)
    {
        bgTime += 0.f; // already updated in draw()
        starPhase += dt * 1.8f;
        pulseCd = std::max(0.f, pulseCd - dt);
        comboTimer = std::max(0.f, comboTimer - dt);
        if (comboTimer <= 0.f) combo = 0;

        // Shake
        if (shakeDur > 0.f) {
            shakeDur -= dt;
            shakeX = fr(-5.f, 5.f);
            shakeY = fr(-5.f, 5.f);
        } else {
            shakeX = shakeY = 0.f;
        }

        // Star follows mouse with smooth inertia
        float mx = (float)p5::mouseX;
        float my = (float)p5::mouseY;
        float ff = std::min(1.f, STAR_FOLLOW * dt);
        starX += (mx - starX) * ff;
        starY += (my - starY) * ff;

        // Pulse ring expansion
        if (pulseR >= 0.f) {
            pulseR += PULSE_SPD * dt;
            if (pulseR > PULSE_MAX_R) pulseR = -1.f;
        }

        // Star mass scales with score (stronger gravity as you improve)
        starMass = 1.0f + std::sqrt((float)score) * 0.055f;

        updateSeeds(dt);
        updateVoidRifts(dt);
        updatePortals(dt);
        updateParticles(dt);
        updateRings(dt);
        updateFloatTexts(dt);
        updateSpawner(dt);

        if (lives <= 0) {
            lives = 0;
            if (score > hiScore) hiScore = score;
            state = State::GameOver;
        }
    }

    // ── Seeds ─────────────────────────────────────────────────────────────────
    void updateSeeds(float dt)
    {
        for (auto& s : seeds) {
            if (!s.alive) continue;

            s.life -= dt;
            s.wobble += dt * 1.9f;

            // Gravity toward star
            float dx = starX - s.x;
            float dy = starY - s.y;
            float dist = std::hypot(dx, dy);
            if (dist > 1.f) {
                float grav = std::min(G_CONST * starMass / (dist * dist), 260.f);
                s.vx += dx / dist * grav * dt;
                s.vy += dy / dist * grav * dt;
            }

            // Pulse boost: ring sweeps through seed
            if (pulseR >= 0.f) {
                float prevR = pulseR - PULSE_SPD * dt;
                float sd = std::hypot(dx, dy);
                if (sd >= prevR - 20.f && sd <= pulseR + 20.f) {
                    float boost = 300.f;
                    s.vx += dx / std::max(dist, 1.f) * boost;
                    s.vy += dy / std::max(dist, 1.f) * boost;
                }
            }

            // Void rift: attracts seed (danger)
            for (auto& vr : voidRifts) {
                if (!vr.alive) continue;
                float vdx = vr.x - s.x;
                float vdy = vr.y - s.y;
                float vd = std::hypot(vdx, vdy);
                if (vd < 2.f) vd = 2.f;
                float vf = std::min(VOID_PULL / (vd * vd), 160.f);
                s.vx += vdx / vd * vf * dt;
                s.vy += vdy / vd * vf * dt;
                if (vd < vr.size + s.size * 0.5f) {
                    s.alive = false;
                    seedLost(s);
                    break;
                }
            }
            if (!s.alive) continue;

            // Speed cap
            float spd = std::hypot(s.vx, s.vy);
            if (spd > 330.f) {
                s.vx = s.vx / spd * 330.f;
                s.vy = s.vy / spd * 330.f;
            }

            s.x += s.vx * dt;
            s.y += s.vy * dt;

            // Trail (fixed rate to avoid physics dependency on framerate)
            s.trailTimer -= dt;
            if (s.trailTimer <= 0.f) {
                s.trailTimer = 0.03f;
                s.trail[s.trailHead % 36] = {s.x, s.y};
                s.trailHead = (s.trailHead + 1) % 36;
                s.trailCount = std::min(s.trailCount + 1, 36);
            }

            // Portal collection
            for (auto& p : portals) {
                if (std::hypot(s.x - p.x, s.y - p.y) < PORTAL_R) {
                    s.alive = false;
                    seedCollected(s, p);
                    break;
                }
            }

            // Timeout
            if (s.life <= 0.f) {
                s.alive = false;
                seedLost(s);
            }
        }
        std::erase_if(seeds, [](const Seed& s) {
            return !s.alive;
        });
    }

    void seedCollected(const Seed& s, Portal& p)
    {
        combo++;
        comboTimer = 2.2f;
        p.pulseT = 1.0f;
        int pts = 50 * combo;
        score += pts;
        color_t sc = hsv(s.hue, 0.85f, 1.0f);
        color_t pc = hsv(p.hue, 0.75f, 1.0f);

        spawnParticles(s.x, s.y, 20, sc);
        spawnParticles(s.x, s.y, 6, rgba(255, 255, 255, 200));
        rings.push_back({s.x, s.y, 0.f, 60.f, 0.5f, 0.5f, sc});
        rings.push_back({p.x, p.y, PORTAL_R * 0.5f, PORTAL_R * 2.8f, 0.7f, 0.7f, pc});

        std::string txt = "+" + std::to_string(pts);
        if (combo > 1) txt += "  ×" + std::to_string(combo);
        floatTexts.push_back({p.x, p.y - 20.f, -38.f, 1.8f, 1.8f, txt, sc});
    }

    void seedLost(const Seed& s)
    {
        lives--;
        shakeDur = std::max(shakeDur, 0.35f);
        combo = 0;
        color_t sc = hsv(s.hue, 0.5f, 0.8f);
        spawnParticles(s.x, s.y, 12, sc);
        spawnParticles(s.x, s.y, 5, rgba(200, 60, 60, 180));
        rings.push_back({s.x, s.y, 0.f, 50.f, 0.6f, 0.6f, rgba(200, 50, 50)});
        floatTexts.push_back({s.x, s.y - 10.f, -32.f, 1.2f, 1.2f, "LOST", rgba(255, 70, 50)});
    }

    // ── Void Rifts ────────────────────────────────────────────────────────────
    void updateVoidRifts(float dt)
    {
        for (auto& vr : voidRifts) {
            if (!vr.alive) continue;
            vr.life -= dt;
            vr.angle += dt * 2.2f;
            vr.x += vr.vx * dt;
            vr.y += vr.vy * dt;
            if (vr.x < 55.f || vr.x > W - 55.f) vr.vx = -vr.vx;
            if (vr.y < 55.f || vr.y > H - 55.f) vr.vy = -vr.vy;
            if (vr.life <= 0.f) vr.alive = false;
        }
        std::erase_if(voidRifts, [](const VoidRift& v) {
            return !v.alive;
        });
    }

    // ── Portals ───────────────────────────────────────────────────────────────
    void updatePortals(float dt)
    {
        for (auto& p : portals) {
            p.angle += dt * 0.75f;
            p.pulseT = std::max(0.f, p.pulseT - dt * 2.2f);
        }
    }

    // ── Spawner ───────────────────────────────────────────────────────────────
    void updateSpawner(float dt)
    {
        seedTimer -= dt;
        if (seedTimer <= 0.f) {
            seedTimer = std::max(0.9f, seedIv - score * 0.004f);
            if ((int)seeds.size() < 6 + score / 200) spawnSeed();
        }
        voidTimer -= dt;
        if (voidTimer <= 0.f && (int)voidRifts.size() < 2) {
            voidTimer = fr(10.f, 16.f);
            spawnVoidRift();
        }
    }

    void spawnSeed()
    {
        Seed s;
        float edge = fr(0.f, 4.f);
        float m = 24.f;
        if (edge < 1.f) {
            s.x = fr(m, W - m);
            s.y = -m;
        } else if (edge < 2.f) {
            s.x = fr(m, W - m);
            s.y = H + m;
        } else if (edge < 3.f) {
            s.x = -m;
            s.y = fr(m, H - m);
        } else {
            s.x = W + m;
            s.y = fr(m, H - m);
        }

        float cx = W * 0.5f + fr(-120.f, 120.f);
        float cy = H * 0.5f + fr(-120.f, 120.f);
        float dx = cx - s.x, dy = cy - s.y;
        float len = std::hypot(dx, dy);
        float spd = fr(22.f, 50.f);
        s.vx = dx / len * spd;
        s.vy = dy / len * spd;
        s.life = s.maxLife = SEED_LIFE;
        s.hue = fr(0.f, 360.f);
        s.size = fr(5.5f, 9.f);
        s.alive = true;
        seeds.push_back(s);
    }

    void spawnVoidRift()
    {
        VoidRift vr;
        vr.x = fr(90.f, W - 90.f);
        vr.y = fr(90.f, H - 90.f);
        float a = fr(0.f, TAU);
        float s = fr(14.f, 30.f);
        vr.vx = std::cos(a) * s;
        vr.vy = std::sin(a) * s;
        vr.life = vr.maxLife = fr(13.f, 21.f);
        vr.size = fr(22.f, 36.f);
        vr.alive = true;
        voidRifts.push_back(vr);
        rings.push_back({vr.x, vr.y, 0.f, 90.f, 0.8f, 0.8f, rgba(110, 0, 160)});
    }

    // ── Effects ───────────────────────────────────────────────────────────────
    void spawnParticles(float x, float y, int n, color_t col)
    {
        for (int i = 0; i < n; ++i) {
            float a = fr(0.f, TAU);
            float s = fr(40.f, 185.f);
            float l = fr(0.25f, 0.65f);
            particles.push_back({x, y, std::cos(a) * s, std::sin(a) * s, l, l, fr(1.5f, 4.5f), col});
        }
    }

    void updateParticles(float dt)
    {
        for (auto& p : particles) {
            p.x += p.vx * dt;
            p.y += p.vy * dt;
            p.vx *= 1.f - dt * 2.8f;
            p.vy *= 1.f - dt * 2.8f;
            p.life -= dt;
        }
        std::erase_if(particles, [](const Particle& p) {
            return p.life <= 0.f;
        });
    }

    void updateRings(float dt)
    {
        for (auto& r : rings) r.life -= dt;
        std::erase_if(rings, [](const Ring& r) {
            return r.life <= 0.f;
        });
    }

    void updateFloatTexts(float dt)
    {
        for (auto& f : floatTexts) {
            f.y += f.vy * dt;
            f.life -= dt;
        }
        std::erase_if(floatTexts, [](const FloatText& f) {
            return f.life <= 0.f;
        });
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Rendering
    // ─────────────────────────────────────────────────────────────────────────
    void renderBg()
    {
        pushCanvas(bgCanvas);
        shader(bgShader);
        setUniform("uTime", uniform(bgTime));
        setUniform("uRes", uniform((float)W, (float)H));
        noStroke();
        fill(255);
        rect(0.f, 0.f, (float)W, (float)H);
        noShader();
        popCanvas();
        blendMode(BlendMode::alpha);
        noTint();
        image(bgCanvas->getTextureId(), 0.f, 0.f, (float)W, (float)H);
    }

    // ── Twinkling background stars ────────────────────────────────────────────
    void drawBgStars()
    {
        blendMode(BlendMode::additive);
        noStroke();
        for (auto& s : bgStars) {
            float tw = 0.5f + 0.5f * std::sin(bgTime * 1.15f + s.phase);
            int a = (int)(s.brightness * tw * 195.f);
            fill(205, 220, 255, a);
            circle(s.x, s.y, s.size * 2.f);
        }
        blendMode(BlendMode::alpha);
    }

    // ── Full scene ────────────────────────────────────────────────────────────
    void drawScene(bool withHud)
    {
        pushMatrix();
        translate(shakeX, shakeY);

        drawPortals();
        drawVoidRifts();

        // Shockwave rings
        blendMode(BlendMode::additive);
        noFill();
        for (auto& r : rings) {
            float t = r.life / r.maxLife;
            float rad = r.curR();
            for (int g = 3; g >= 0; --g) {
                stroke(red(r.col), green(r.col), blue(r.col), (int)(t * 140.f / (g + 1)));
                strokeWeight(2.2f - g * 0.3f);
                circle(r.x, r.y, (rad + g * 6.f) * 2.f);
            }
        }

        // Particles
        noStroke();
        for (auto& p : particles) {
            float t = p.life / p.maxLife;
            fill(red(p.col), green(p.col), blue(p.col), (int)(t * 200.f));
            circle(p.x, p.y, p.size * 2.f * (0.4f + 0.6f * t));
        }
        blendMode(BlendMode::alpha);

        drawSeeds();

        // Pulse ring
        if (pulseR >= 0.f) {
            blendMode(BlendMode::additive);
            noFill();
            float frac = 1.f - pulseR / PULSE_MAX_R;
            for (int g = 3; g >= 0; --g) {
                stroke(255, 225, 80, (int)(frac * 70.f / (g + 1)));
                strokeWeight(2.f + g * 1.5f);
                circle(starX, starY, (pulseR + g * 9.f) * 2.f);
            }
            blendMode(BlendMode::alpha);
        }

        drawPlayerStar();

        if (withHud) drawHUD();
        drawFloatTexts();
        popMatrix();
    }

    // ── Portals ───────────────────────────────────────────────────────────────
    void drawPortals()
    {
        for (auto& p : portals) {
            color_t pc = hsv(p.hue, 0.75f, 1.0f);
            int cr = red(pc), cg = green(pc), cb = blue(pc);
            float flash = p.pulseT;

            blendMode(BlendMode::additive);
            noStroke();

            // Soft glow layers
            for (int g = 5; g >= 0; --g) {
                float spread = g * 10.f + flash * 28.f;
                int a = (int)((35 - g * 4) + flash * 55.f);
                fill(cr, cg, cb, std::max(0, a));
                circle(p.x, p.y, (PORTAL_R + spread) * 2.f);
            }

            // Outer ring
            noFill();
            stroke(cr, cg, cb, 160 + (int)(flash * 80.f));
            strokeWeight(2.2f);
            circle(p.x, p.y, PORTAL_R * 2.f);

            // Rotating sigil (6-pointed star made from two triangles)
            noStroke();
            pushMatrix();
            translate(p.x, p.y);
            for (int tri = 0; tri < 2; ++tri) {
                rotate(p.angle + tri * PI / 3.f);
                float r = PORTAL_R * 0.55f;
                fill(cr, cg, cb, 190);
                beginShape();
                for (int i = 0; i < 3; ++i) {
                    float a = TAU * i / 3.f;
                    vertex(std::cos(a) * r, std::sin(a) * r);
                }
                endShape(ShapeType::polygon);
            }
            popMatrix();

            // Inner bright core
            noStroke();
            fill(cr, cg, cb, 140 + (int)(flash * 100.f));
            circle(p.x, p.y, PORTAL_R * 0.4f);

            blendMode(BlendMode::alpha);

            // Dashed orbit ring (alpha blend)
            noFill();
            stroke(cr, cg, cb, 55);
            strokeWeight(1.f);
            circle(p.x, p.y, PORTAL_R * 2.7f);
        }
    }

    // ── Void Rifts ────────────────────────────────────────────────────────────
    void drawVoidRifts()
    {
        for (auto& vr : voidRifts) {
            if (!vr.alive) continue;
            float lt = vr.life / vr.maxLife;
            float alpha = (lt > 0.18f) ? 1.f : lt / 0.18f;

            blendMode(BlendMode::additive);
            noStroke();

            // Purple glow halo
            for (int g = 5; g >= 0; --g) {
                float sp = g * 8.f;
                fill(70 - g * 8, 0, 110 + g * 8, (int)(alpha * (25 + g * 9)));
                circle(vr.x, vr.y, (vr.size + sp) * 2.f);
            }

            // Spiral arms
            pushMatrix();
            translate(vr.x, vr.y);
            rotate(vr.angle);
            noFill();
            stroke(130, 0, 190, (int)(alpha * 175.f));
            strokeWeight(1.8f);
            for (int arm = 0; arm < 3; ++arm) {
                float base = TAU * arm / 3.f;
                beginShape();
                for (int j = 0; j <= 22; ++j) {
                    float t = (float)j / 22.f;
                    float r = vr.size * t;
                    float a = base + t * TAU * 0.65f;
                    vertex(std::cos(a) * r, std::sin(a) * r);
                }
                endShape(ShapeType::lineStrip, false);
            }
            // Dark anti-gravity core
            noStroke();
            fill(15, 0, 30, (int)(alpha * 200.f));
            circle(0.f, 0.f, vr.size * 0.8f);
            popMatrix();

            blendMode(BlendMode::alpha);
        }
    }

    // ── Seeds ─────────────────────────────────────────────────────────────────
    void drawSeeds()
    {
        blendMode(BlendMode::additive);
        noStroke();
        for (auto& s : seeds) {
            if (!s.alive) continue;
            color_t sc = hsv(s.hue, 0.85f, 1.0f);
            int cr = red(sc), cg = green(sc), cb = blue(sc);

            float lifeT = s.life / s.maxLife;
            float urgency = (lifeT < 0.28f) ? (0.5f + 0.5f * std::sin(bgTime * 13.f)) : 1.f;

            // Trail
            for (int i = 0; i < s.trailCount; ++i) {
                int idx = (s.trailHead - 1 - i + 36) % 36;
                float t = 1.f - (float)i / (float)s.trailCount;
                fill(cr, cg, cb, (int)(t * t * 95.f));
                circle(s.trail[idx].x, s.trail[idx].y, s.size * t * 1.6f);
            }

            // Glow layers
            for (int g = 3; g >= 0; --g) {
                fill(cr, cg, cb, (int)(urgency * (14 + g * 11)));
                circle(s.x, s.y, (s.size + g * 5.5f) * 2.f);
            }

            // Bright core
            fill(255, 255, 255, (int)(urgency * 230.f));
            circle(s.x, s.y, s.size * 0.75f);

            // Orbiting corona points
            fill(cr, cg, cb, (int)(urgency * 175.f));
            pushMatrix();
            translate(s.x, s.y);
            rotate(s.wobble);
            for (int k = 0; k < 4; ++k) {
                float a = TAU * k / 4.f;
                float r = s.size * 0.65f + std::sin(s.wobble * 2.3f + k) * 2.f;
                circle(std::cos(a) * r, std::sin(a) * r, 4.5f);
            }
            popMatrix();
        }
        blendMode(BlendMode::alpha);
    }

    // ── Player star ───────────────────────────────────────────────────────────
    void drawPlayerStar()
    {
        float corona = 0.5f + 0.5f * std::sin(starPhase * 1.1f);
        float baseR = std::min(18.f + starMass * 3.2f, 42.f);

        blendMode(BlendMode::additive);
        noStroke();

        // Outer corona halo
        for (int g = 7; g >= 0; --g) {
            float sp = g * 11.f + corona * 7.f;
            fill(255, 195 - g * 12, 55, (int)((18 + corona * 9) * (8 - g) / 8));
            circle(starX, starY, (baseR + sp) * 2.f);
        }

        // Coronal rays (6 spikes, slowly rotating)
        pushMatrix();
        translate(starX, starY);
        rotate(starPhase * 0.28f);
        fill(255, 225, 100, 155);
        for (int i = 0; i < 6; ++i) {
            float ang = TAU * i / 6.f;
            float rayLen = baseR * (1.45f + 0.3f * std::sin(starPhase * 2.1f + i));
            float hw = 3.2f;
            beginShape();
            vertex(std::cos(ang) * baseR * 0.55f - std::sin(ang) * hw, std::sin(ang) * baseR * 0.55f + std::cos(ang) * hw);
            vertex(std::cos(ang) * rayLen, std::sin(ang) * rayLen);
            vertex(std::cos(ang) * baseR * 0.55f + std::sin(ang) * hw, std::sin(ang) * baseR * 0.55f - std::cos(ang) * hw);
            endShape(ShapeType::polygon);
        }
        popMatrix();

        // Core sphere
        fill(255, 245, 165, 235);
        circle(starX, starY, baseR * 2.f);

        // White hot center
        fill(255, 255, 255, 220);
        circle(starX, starY, baseR * 0.55f);

        blendMode(BlendMode::alpha);
    }

    // ── HUD ───────────────────────────────────────────────────────────────────
    void drawHUD()
    {
        // Score (top center)
        textAlign(TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::top});
        fill(185, 215, 255, 190);
        textSize(12.f);
        text("SCORE", (float)W * 0.5f, 13.f);
        fill(255, 230, 90, 235);
        textSize(26.f);
        text(std::format("{:06d}", score), (float)W * 0.5f, 28.f);

        // Combo
        if (combo > 1) {
            float pulse = 0.5f + 0.5f * std::sin(bgTime * 7.f);
            fill(100, 255, 175, (int)(comboTimer / 2.2f * 210.f));
            textSize(15.f + pulse * 3.f);
            text("×" + std::to_string(combo) + " COMBO", (float)W * 0.5f, 60.f);
        }

        // Hi-score (top right)
        textAlign(TextAlign {.horizontal = HorizontalTextAlign::right, .vertical = VerticalTextAlign::top});
        fill(130, 175, 255, 130);
        textSize(10.f);
        text("BEST  " + std::format("{:06d}", hiScore), (float)W - 13.f, 13.f);

        // Lives (top left, glowing dots)
        for (int i = 0; i < 5; ++i) {
            float lx = 20.f + i * 22.f;
            float ly = 22.f;
            blendMode(BlendMode::additive);
            noStroke();
            if (i < lives) {
                fill(255, 200, 60, 55);
                circle(lx, ly, 18.f);
                fill(255, 235, 110, 210);
            } else {
                fill(55, 55, 95, 110);
            }
            circle(lx, ly, 8.f);
            blendMode(BlendMode::alpha);
        }

        // Pulse cooldown arc (bottom left)
        float pcFrac = 1.f - (pulseCd / PULSE_CD);
        float pcX = 44.f, pcY = (float)H - 44.f, pcR = 21.f;

        blendMode(BlendMode::additive);
        noFill();
        stroke(35, 35, 85, 130);
        strokeWeight(3.5f);
        circle(pcX, pcY, pcR * 2.f);

        if (pulseCd > 0.f) {
            stroke(255, 200, 60, 185);
            strokeWeight(3.5f);
            beginShape();
            int segs = 40;
            for (int i = 0; i <= segs; ++i) {
                float a = -PI * 0.5f + TAU * pcFrac * (float)i / segs;
                vertex(pcX + std::cos(a) * pcR, pcY + std::sin(a) * pcR);
            }
            endShape(ShapeType::lineStrip, false);
        } else {
            float pp = 0.5f + 0.5f * std::sin(bgTime * 4.f);
            stroke(255, 225, 75, (int)(155 + pp * 90.f));
            strokeWeight(3.5f);
            circle(pcX, pcY, pcR * 2.f);
            noStroke();
            fill(255, 215, 60, (int)(pp * 35.f));
            circle(pcX, pcY, pcR * 2.f);
        }
        blendMode(BlendMode::alpha);

        noStroke();
        fill(195, 215, 255, pulseCd <= 0.f ? 215 : 130);
        textAlign(TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::center});
        textSize(8.f);
        text("PULSE", pcX, pcY - 3.f);
        text("CLICK", pcX, pcY + 6.f);

        // Star mass indicator (bottom right)
        textAlign(TextAlign {.horizontal = HorizontalTextAlign::right, .vertical = VerticalTextAlign::top});
        fill(255, 200, 80, 150);
        textSize(10.f);
        text(std::format("MASS {:.1f}×", starMass), (float)W - 13.f, (float)H - 26.f);
    }

    // ── Float texts ───────────────────────────────────────────────────────────
    void drawFloatTexts()
    {
        textAlign(TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::center});
        for (auto& ft : floatTexts) {
            float t = ft.life / ft.maxLife;
            noStroke();
            fill(0, 0, 0, (int)(t * 75.f));
            textSize(14.f);
            text(ft.text, ft.x + 1.f, ft.y + 1.f);
            fill(red(ft.col), green(ft.col), blue(ft.col), (int)(t * 225.f));
            text(ft.text, ft.x, ft.y);
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Menu
    // ─────────────────────────────────────────────────────────────────────────
    void drawMenu()
    {
        textAlign(TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::center});

        float pulse = 0.5f + 0.5f * std::sin(bgTime * 1.1f);

        blendMode(BlendMode::additive);
        for (int g = 5; g >= 0; --g) {
            fill(255, 185, 45, (int)((13 + pulse * 9) * (6 - g)));
            textSize(84.f + g * 3.f);
            text("GRAVITAS", (float)W * 0.5f, (float)H * 0.3f);
        }
        blendMode(BlendMode::alpha);
        fill(255, 240, 170, 248);
        textSize(84.f);
        text("GRAVITAS", (float)W * 0.5f, (float)H * 0.3f);

        fill(175, 200, 255, 165);
        textSize(13.f);
        text("STELLAR GRAVITY PUZZLE", (float)W * 0.5f, (float)H * 0.3f + 55.f);

        float hy = (float)H * 0.54f;
        fill(145, 190, 255, 155);
        textSize(12.f);
        text("Move your  STAR  with the mouse  —  gravity attracts cosmic seeds", (float)W * 0.5f, hy);
        text("Guide seeds into the glowing  PORTALS  to score points", (float)W * 0.5f, hy + 22.f);
        text("Left Click  —  Gravitational Pulse  (boosts seeds toward you)", (float)W * 0.5f, hy + 44.f);
        text("Avoid  VOID RIFTS  that steal seeds away", (float)W * 0.5f, hy + 66.f);

        // Portals preview
        for (auto& p : portals) {
            color_t pc = hsv(p.hue, 0.7f, 1.0f);
            blendMode(BlendMode::additive);
            noStroke();
            fill(red(pc), green(pc), blue(pc), 55);
            circle(p.x, p.y, PORTAL_R * 2.f);
            fill(red(pc), green(pc), blue(pc), 140);
            circle(p.x, p.y, PORTAL_R * 0.45f);
            blendMode(BlendMode::alpha);
        }

        float blink = 0.5f + 0.5f * std::sin(bgTime * 2.5f);
        fill(100, 215, 255, (int)(blink * 225.f));
        textSize(16.f);
        text("PRESS ENTER  OR  CLICK TO START", (float)W * 0.5f, (float)H * 0.81f);

        if (hiScore > 0) {
            fill(255, 220, 80, 155);
            textSize(12.f);
            text("BEST  " + std::format("{:06d}", hiScore), (float)W * 0.5f, (float)H * 0.88f);
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Game Over
    // ─────────────────────────────────────────────────────────────────────────
    void drawGameOver()
    {
        noStroke();
        fill(0, 0, 18, 165);
        rect(0.f, 0.f, W, H);

        textAlign(TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::center});

        blendMode(BlendMode::additive);
        for (int g = 4; g >= 0; --g) {
            fill(255, 55, 35, 15 * (5 - g));
            textSize(64.f + g * 2.f);
            text("COLLAPSED", (float)W * 0.5f, (float)H * 0.32f);
        }
        blendMode(BlendMode::alpha);
        fill(255, 110, 75, 245);
        textSize(64.f);
        text("COLLAPSED", (float)W * 0.5f, (float)H * 0.32f);

        fill(255, 230, 80, 235);
        textSize(24.f);
        text(std::format("{:06d}", score), (float)W * 0.5f, (float)H * 0.485f);

        if (score > 0 && score >= hiScore) {
            float p = 0.5f + 0.5f * std::sin(bgTime * 3.f);
            fill(255, 220, 40, (int)(185 + p * 65.f));
            textSize(15.f);
            text("NEW HIGH SCORE!", (float)W * 0.5f, (float)H * 0.555f);
        } else if (hiScore > 0) {
            fill(135, 190, 255, 155);
            textSize(13.f);
            text("BEST  " + std::format("{:06d}", hiScore), (float)W * 0.5f, (float)H * 0.555f);
        }

        float blink = 0.5f + 0.5f * std::sin(bgTime * 2.5f);
        fill(95, 205, 255, (int)(blink * 215.f));
        textSize(16.f);
        text("PRESS ENTER  OR  CLICK TO RETRY", (float)W * 0.5f, (float)H * 0.77f);
    }
};

std::unique_ptr<p5::Sketch> p5::createSketch()
{
    return std::make_unique<GravitasSketch>();
}
