#include "p5.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <numbers>
#include <random>
#include <string>
#include <vector>

using namespace p5;

static constexpr float PI = std::numbers::pi_v<float>;

// ─────────────────────────────────────────────────────────────────────────────
//  BreakoutSketch
//  Visual features:
//    • Animated neon-grid background rendered via a custom GLSL shader + offscreen canvas
//    • Additive-blended glowing ball with long motion trail
//    • Per-brick pulsating colour halos + top-highlight strip
//    • Crack lines on damaged 2-HP bricks
//    • Particle explosions on brick destruction (additive blend)
//    • Paddle glow burst on ball reflection
//    • Three power-ups with animated glowing capsules (MultiBall / Wide / Slow)
//    • Screen-shake on brick hit
//    • Combo multiplier HUD element (colour-flashing text)
//    • Scrolling star field behind everything
// ─────────────────────────────────────────────────────────────────────────────

struct BreakoutSketch : p5::Sketch
{
    // ── Window / layout ──────────────────────────────────────────────────────
    static constexpr int   W           = 800;
    static constexpr int   H           = 900;

    // ── Physics constants ────────────────────────────────────────────────────
    static constexpr float PADDLE_W    = 110.f;
    static constexpr float PADDLE_H    = 14.f;
    static constexpr float PADDLE_Y    = H - 55.f;
    static constexpr float BALL_R      = 8.f;
    static constexpr float BRICK_W     = 67.f;
    static constexpr float BRICK_H     = 20.f;
    static constexpr float BRICK_GAP   = 5.f;
    static constexpr int   BRICK_COLS  = 10;
    static constexpr int   BRICK_ROWS  = 7;
    static constexpr float BRICKS_LEFT = 25.f;
    static constexpr float BRICKS_TOP  = 90.f;
    static constexpr float BASE_SPEED  = 300.f;
    static constexpr int   TRAIL_LEN   = 24;

    // ── Types ────────────────────────────────────────────────────────────────
    enum class State { Start, Playing, GameOver, Win };

    struct Ball
    {
        float x = 0, y = 0, vx = 0, vy = 0;
        bool  alive = true;
        std::array<float2, TRAIL_LEN> trail{};
        int   trailLen = 0;
    };

    struct Brick
    {
        float   x = 0, y = 0;
        int     hp = 1, maxHp = 1;
        bool    alive = true;
        color_t col   = 0;
        float   phase = 0; // individual glow oscillation offset
    };

    struct Particle
    {
        float   x = 0, y = 0, vx = 0, vy = 0;
        float   life = 1, maxLife = 1, size = 3;
        color_t col  = 0;
    };

    enum class PUType { MultiBall, Wide, Slow };

    struct PowerUp
    {
        float  x = 0, y = 0, vy = 115.f;
        PUType type  = PUType::MultiBall;
        bool   alive = true;
    };

    struct Star { float x = 0, y = 0, speed = 0, sz = 0; };

    // ── State ────────────────────────────────────────────────────────────────
    State state      = State::Start;
    float paddleX    = W / 2.f;
    float paddleGlow = 0.f;
    float wideTimer  = 0.f;
    float slowTimer  = 0.f;
    int   score      = 0;
    int   lives      = 3;
    int   combo      = 0;
    float comboTimer = 0.f;
    float shakeAmt   = 0.f;
    float shakeX     = 0.f;
    float shakeY     = 0.f;
    float t          = 0.f; // accumulated seconds

    std::vector<Ball>     balls;
    std::vector<Brick>    bricks;
    std::vector<Particle> particles;
    std::vector<PowerUp>  powerUps;
    std::vector<Star>     stars;

    std::shared_ptr<Shader>      bgShader;
    std::shared_ptr<Framebuffer> bgCanvas;

    std::mt19937 rng{ std::random_device{}() };

    // ── Helpers ──────────────────────────────────────────────────────────────
    float rnd(float lo, float hi)
    {
        return std::uniform_real_distribution<float>(lo, hi)(rng);
    }

    static color_t hsv(float h, float s, float v, int a = 255)
    {
        h = std::fmod(h, 360.f);
        if (h < 0.f) h += 360.f;
        float r, g, b;
        const int   i = int(h / 60.f) % 6;
        const float f = h / 60.f - int(h / 60.f);
        const float p = v * (1.f - s);
        const float q = v * (1.f - f * s);
        const float u = v * (1.f - (1.f - f) * s);
        switch (i) {
            case 0: r=v; g=u; b=p; break;
            case 1: r=q; g=v; b=p; break;
            case 2: r=p; g=v; b=u; break;
            case 3: r=p; g=q; b=v; break;
            case 4: r=u; g=p; b=v; break;
            default: r=v; g=p; b=q; break;
        }
        return color(int(r * 255), int(g * 255), int(b * 255), a);
    }

    float currentPaddleHW() const
    {
        return (wideTimer > 0.f ? PADDLE_W * 1.65f : PADDLE_W) * 0.5f;
    }

    // ── setup ────────────────────────────────────────────────────────────────
    void setup() override
    {
        setWindowSize(W, H);
        setWindowTitle("BREAKOUT  ─  p5 Edition");
        setWindowResizable(false);
        frameRate(60);
        buildBgShader();
        bgCanvas = std::shared_ptr<Framebuffer>(createCanvas(W, H).release());
        initStars();
        initGame();
    }

    void destroy() override {}

    void buildBgShader()
    {
        // Vertex shader matches the library's attribute layout exactly.
        static constexpr const char* VERT = R"(
            #version 410 core
            layout(location=0) in vec2  a_Position;
            layout(location=1) in vec2  a_TexCoord;
            layout(location=2) in vec4  a_Color;
            layout(location=3) in float a_TexIndex;
            out vec2  v_TexCoord;
            out vec4  v_Color;
            out float v_TexIndex;
            uniform mat4 u_ProjectionMatrix;
            void main() {
                gl_Position = u_ProjectionMatrix * vec4(a_Position, 0.0, 1.0);
                v_TexCoord  = a_TexCoord;
                v_Color     = a_Color;
                v_TexIndex  = a_TexIndex;
            }
        )";

        // Fragment shader uses gl_FragCoord so it works regardless of UV data.
        static constexpr const char* FRAG = R"(
            #version 410 core
            layout(location=0) out vec4 o_Color;
            uniform float uTime;
            uniform vec2  uResolution;

            void main() {
                vec2 uv = gl_FragCoord.xy / uResolution;
                uv.y    = 1.0 - uv.y;   // flip: top=0

                // ── Dark space gradient ──────────────────────────────────
                vec3 col = mix(vec3(0.01, 0.02, 0.08), vec3(0.03, 0.06, 0.14), uv.y);

                // ── Neon grid ────────────────────────────────────────────
                vec2 cell  = fract(uv * vec2(14.0, 17.0));
                float lx   = smoothstep(0.0, 0.06, cell.x) * smoothstep(1.0, 0.94, cell.x);
                float ly   = smoothstep(0.0, 0.06, cell.y) * smoothstep(1.0, 0.94, cell.y);
                float grid = 1.0 - lx * ly;
                float pulse = 0.5 + 0.5 * sin(uTime * 1.4);
                vec3  gc    = mix(vec3(0.0, 0.18, 0.55), vec3(0.0, 0.48, 1.0), pulse);
                col         = mix(col, gc, grid * (0.14 + 0.08 * pulse));

                // ── Horizontal accent glow near the brick-field horizon ──
                float hd = abs(uv.y - 0.38);
                col += vec3(0.0, 0.25, 0.7) * exp(-hd * 14.0) * (0.04 + 0.02 * pulse);

                // ── Vignette ─────────────────────────────────────────────
                vec2  vig = uv * 2.0 - 1.0;
                col *= 1.0 - dot(vig, vig) * 0.30;

                // ── Moving scanline ──────────────────────────────────────
                float scan = fract(uv.y - uTime * 0.05);
                col *= 1.0 - 0.12 * smoothstep(0.0, 0.02, scan)
                                  * smoothstep(0.04, 0.02, scan);

                o_Color = vec4(col, 1.0);
            }
        )";

        bgShader = std::shared_ptr<Shader>(loadShader(VERT, FRAG).release());
    }

    void initStars()
    {
        stars.resize(160);
        for (auto& s : stars) {
            s.x     = rnd(0.f, float(W));
            s.y     = rnd(0.f, float(H));
            s.speed = rnd(8.f, 55.f);
            s.sz    = rnd(0.4f, 2.6f);
        }
    }

    // ── Game lifecycle ───────────────────────────────────────────────────────
    void initGame()
    {
        score = 0; lives = 3; combo = 0;
        comboTimer = 0.f; shakeAmt = 0.f;
        wideTimer  = 0.f; slowTimer = 0.f; paddleGlow = 0.f;
        balls.clear(); particles.clear(); powerUps.clear();
        spawnBall();
        buildBricks();
    }

    void spawnBall(float fromX      = float(W) / 2.f,
                   float fromY      = PADDLE_Y - BALL_R - 4.f,
                   float extraAngle = 0.f)
    {
        Ball b;
        b.x = fromX; b.y = fromY;
        float a = -PI / 2.f + extraAngle + rnd(-0.4f, 0.4f);
        b.vx = BASE_SPEED * std::cos(a);
        b.vy = BASE_SPEED * std::sin(a);
        balls.push_back(b);
    }

    void buildBricks()
    {
        bricks.clear();
        for (int row = 0; row < BRICK_ROWS; ++row) {
            for (int col = 0; col < BRICK_COLS; ++col) {
                Brick br;
                br.x     = BRICKS_LEFT + col * (BRICK_W + BRICK_GAP);
                br.y     = BRICKS_TOP  + row * (BRICK_H + BRICK_GAP);
                br.maxHp = br.hp = (row < 2) ? 2 : 1;
                br.col   = hsv(float(row) * (360.f / BRICK_ROWS) + 10.f, 0.85f, 1.f);
                br.phase = rnd(0.f, 2.f * PI);
                bricks.push_back(br);
            }
        }
    }

    // ── Update ───────────────────────────────────────────────────────────────
    void update(float dt)
    {
        comboTimer -= dt;
        if (comboTimer <= 0.f) combo = 0;

        wideTimer  = std::max(0.f, wideTimer - dt);
        slowTimer  = std::max(0.f, slowTimer - dt);
        paddleGlow = std::max(0.f, paddleGlow - dt * 3.f);

        // Paddle follows mouse
        float pHW = currentPaddleHW();
        paddleX   = std::clamp(float(mouseX), pHW, float(W) - pHW);

        // Screen-shake decay
        if (shakeAmt > 0.f) {
            shakeAmt -= dt * 7.f;
            shakeX = rnd(-shakeAmt * 7.f, shakeAmt * 7.f);
            shakeY = rnd(-shakeAmt * 7.f, shakeAmt * 7.f);
        } else { shakeX = shakeY = 0.f; }

        const float sm = slowTimer > 0.f ? 0.55f : 1.f;

        for (auto& ball : balls) {
            if (!ball.alive) continue;
            ball.x += ball.vx * dt * sm;
            ball.y += ball.vy * dt * sm;

            // Trail: shift array right and write new head
            for (int i = std::min(ball.trailLen, TRAIL_LEN - 1); i > 0; --i)
                ball.trail[i] = ball.trail[i - 1];
            ball.trail[0] = { ball.x, ball.y };
            if (ball.trailLen < TRAIL_LEN) ++ball.trailLen;

            // Wall bounce
            if (ball.x - BALL_R < 0.f)       { ball.x = BALL_R;           ball.vx =  std::abs(ball.vx); }
            if (ball.x + BALL_R > float(W))   { ball.x = float(W) - BALL_R; ball.vx = -std::abs(ball.vx); }
            if (ball.y - BALL_R < 0.f)        { ball.y = BALL_R;           ball.vy =  std::abs(ball.vy); }

            // Paddle collision
            float pL = paddleX - pHW, pR = paddleX + pHW;
            float pT = PADDLE_Y - PADDLE_H * 0.5f;
            if (ball.vy > 0.f
                && ball.y + BALL_R >= pT
                && ball.y - BALL_R <= pT + PADDLE_H + 5.f
                && ball.x >= pL - 2.f
                && ball.x <= pR + 2.f)
            {
                float hit = (ball.x - paddleX) / pHW; // -1 .. +1
                float sp  = std::sqrt(ball.vx * ball.vx + ball.vy * ball.vy);
                float an  = hit * PI / 2.8f;
                ball.vx   = sp * std::sin(an);
                ball.vy   = -sp * std::cos(an);
                ball.y    = pT - BALL_R - 0.5f;
                paddleGlow = 1.f;
                spawnBurst(ball.x, pT, color(120, 210, 255), 9, false);
            }

            // Brick collisions (SAT on AABB, smallest-overlap axis)
            for (auto& br : bricks) {
                if (!br.alive) continue;
                float bL = br.x, bR = br.x + BRICK_W;
                float bT = br.y, bB = br.y + BRICK_H;
                float oL = ball.x + BALL_R - bL;
                float oR = bR - (ball.x - BALL_R);
                float oT = ball.y + BALL_R - bT;
                float oB = bB - (ball.y - BALL_R);
                if (oL < 0 || oR < 0 || oT < 0 || oB < 0) continue;

                float mo = std::min({ oL, oR, oT, oB });
                if      (mo == oL) { ball.vx = -std::abs(ball.vx); ball.x = bL - BALL_R; }
                else if (mo == oR) { ball.vx =  std::abs(ball.vx); ball.x = bR + BALL_R; }
                else if (mo == oT) { ball.vy = -std::abs(ball.vy); ball.y = bT - BALL_R; }
                else               { ball.vy =  std::abs(ball.vy); ball.y = bB + BALL_R; }

                --br.hp;
                ++combo; comboTimer = 2.5f;
                score += 10 * std::max(1, combo / 4);
                shakeAmt = std::min(shakeAmt + 0.22f, 1.f);

                const bool destroyed = br.hp <= 0;
                spawnBurst(br.x + BRICK_W * 0.5f, br.y + BRICK_H * 0.5f,
                           br.col, destroyed ? 22 : 7, destroyed);
                if (destroyed) {
                    br.alive = false;
                    if (rnd(0.f, 1.f) < 0.18f) spawnPU(br);
                }
                break; // one brick per ball per frame
            }

            if (ball.y - BALL_R > float(H)) ball.alive = false;
        }

        // Prune dead balls
        balls.erase(std::remove_if(balls.begin(), balls.end(),
            [](const Ball& b) { return !b.alive; }), balls.end());

        if (balls.empty()) {
            if (--lives <= 0) { state = State::GameOver; return; }
            spawnBall();
        }

        bool anyAlive = false;
        for (const auto& br : bricks) if (br.alive) { anyAlive = true; break; }
        if (!anyAlive) { state = State::Win; return; }

        // Power-ups
        for (auto& pu : powerUps) {
            if (!pu.alive) continue;
            pu.y += pu.vy * dt;
            const float pHalfW = currentPaddleHW();
            if (std::abs(pu.x - paddleX) < pHalfW + 12.f
                && std::abs(pu.y - PADDLE_Y) < PADDLE_H + 12.f)
            {
                applyPU(pu.type); pu.alive = false;
            }
            if (pu.y > float(H) + 20.f) pu.alive = false;
        }
        powerUps.erase(std::remove_if(powerUps.begin(), powerUps.end(),
            [](const PowerUp& p) { return !p.alive; }), powerUps.end());

        // Particles
        for (auto& p : particles) {
            p.x  += p.vx * dt;
            p.y  += p.vy * dt;
            p.vy += 240.f * dt; // gravity
            p.life -= dt;
        }
        particles.erase(std::remove_if(particles.begin(), particles.end(),
            [](const Particle& p) { return p.life <= 0.f; }), particles.end());

        // Scrolling stars
        for (auto& s : stars) {
            s.y += s.speed * dt;
            if (s.y > float(H) + 4.f) { s.y = -4.f; s.x = rnd(0.f, float(W)); }
        }
    }

    void spawnBurst(float x, float y, color_t c, int n, bool big)
    {
        for (int i = 0; i < n; ++i) {
            Particle p;
            p.x    = x + rnd(-BRICK_W * 0.35f, BRICK_W * 0.35f);
            p.y    = y + rnd(-BRICK_H * 0.5f,  BRICK_H * 0.5f);
            float a  = rnd(0.f, 2.f * PI);
            float sp = big ? rnd(70.f, 330.f) : rnd(25.f, 120.f);
            p.vx   = sp * std::cos(a);
            p.vy   = sp * std::sin(a) - (big ? 90.f : 0.f);
            p.life = p.maxLife = rnd(0.25f, big ? 1.1f : 0.55f);
            p.size = big ? rnd(2.f, 7.f) : rnd(1.f, 3.5f);
            p.col  = c;
            particles.push_back(p);
        }
    }

    void spawnPU(const Brick& br)
    {
        PowerUp pu;
        pu.x    = br.x + BRICK_W * 0.5f;
        pu.y    = br.y + BRICK_H;
        pu.type = PUType(int(rnd(0.f, 3.f)));
        powerUps.push_back(pu);
    }

    void applyPU(PUType type)
    {
        switch (type) {
        case PUType::MultiBall: {
            std::vector<Ball> extras;
            for (auto& b : balls) {
                Ball nb  = b; nb.trail = {}; nb.trailLen = 0;
                float sp = std::sqrt(nb.vx * nb.vx + nb.vy * nb.vy);
                float a  = std::atan2(nb.vy, nb.vx) + PI / 5.f;
                nb.vx = sp * std::cos(a);
                nb.vy = sp * std::sin(a);
                extras.push_back(nb);
            }
            for (auto& e : extras) balls.push_back(e);
            break;
        }
        case PUType::Wide: wideTimer = 9.f; break;
        case PUType::Slow: slowTimer = 6.f; break;
        }
    }

    // ── Draw helpers ─────────────────────────────────────────────────────────
    void drawBackground()
    {
        // Render the animated neon-grid shader into the offscreen canvas,
        // then blit it onto the main canvas as a full-screen image.
        pushCanvas(bgCanvas);
        shader(bgShader);
        setUniform("uTime",       uniform(t));
        setUniform("uResolution", uniform(float(W), float(H)));
        noStroke(); fill(255);
        rect(0.f, 0.f, float(W), float(H));
        noShader();
        popCanvas();

        noTint();
        image(bgCanvas->getTextureId(), 0.f, 0.f, float(W), float(H));
    }

    void drawStars()
    {
        blendMode(BlendMode::additive);
        noStroke();
        for (const auto& s : stars) {
            float flicker = 0.55f + 0.45f * std::sin(t * s.speed * 0.25f + s.x * 0.1f);
            fill(210, 225, 255, int(165.f * flicker));
            circle(s.x, s.y, s.sz * 2.f);
        }
        blendMode(BlendMode::alpha);
    }

    void drawBricks()
    {
        for (const auto& br : bricks) {
            if (!br.alive) continue;
            const int   r  = p5::red(br.col);
            const int   g  = p5::green(br.col);
            const int   b  = p5::blue(br.col);
            const float pu = 0.5f + 0.5f * std::sin(t * 1.8f + br.phase);
            const int   dm = (br.hp < br.maxHp) ? 130 : 255; // dim damaged bricks

            // Additive glow halo
            blendMode(BlendMode::additive);
            noStroke();
            fill(r, g, b, int(25.f * pu + 12.f));
            rect(br.x - 6.f, br.y - 6.f, BRICK_W + 12.f, BRICK_H + 12.f, 8.f, 8.f);
            blendMode(BlendMode::alpha);

            // Brick body
            noStroke();
            fill(r * dm / 255, g * dm / 255, b * dm / 255);
            rect(br.x, br.y, BRICK_W, BRICK_H, 4.f, 4.f);

            // Shiny top-edge highlight
            blendMode(BlendMode::additive);
            fill(255, 255, 255, 48);
            rect(br.x + 2.f, br.y + 2.f, BRICK_W - 4.f, 4.f, 2.f, 2.f);
            blendMode(BlendMode::alpha);

            // Zig-zag crack on 2-HP bricks that have been hit once
            if (br.maxHp == 2 && br.hp == 1) {
                noFill();
                stroke(0, 0, 0, 130);
                strokeWeight(1.5f);
                const float cx = br.x + BRICK_W * 0.5f;
                line(cx - 7.f, br.y + 2.f,    cx,       br.y + 8.f);
                line(cx,       br.y + 8.f,    cx - 4.f, br.y + 13.f);
                line(cx - 4.f, br.y + 13.f,   cx + 5.f, br.y + BRICK_H - 2.f);
            }
        }
    }

    void drawBall(const Ball& ball)
    {
        blendMode(BlendMode::additive);
        noStroke();

        // Trail (render oldest → newest so the bright head is on top)
        for (int i = ball.trailLen - 1; i >= 0; --i) {
            const float frac = 1.f - float(i) / float(ball.trailLen);
            const float tr   = BALL_R * frac * 0.8f;
            fill(50, 170, 255, int(160.f * frac * frac));
            circle(ball.trail[i].x, ball.trail[i].y, tr * 2.f);
        }

        // Soft glow rings around the ball
        for (int k = 4; k >= 1; --k) {
            fill(80, 160, 255, 35 / k);
            circle(ball.x, ball.y, (BALL_R + float(k) * 4.5f) * 2.f);
        }

        // Opaque core
        blendMode(BlendMode::alpha);
        noStroke();
        fill(215, 240, 255);
        circle(ball.x, ball.y, BALL_R * 2.f);

        // Specular highlight
        blendMode(BlendMode::additive);
        fill(255, 255, 255, 185);
        circle(ball.x - BALL_R * 0.28f, ball.y - BALL_R * 0.28f, BALL_R * 0.65f * 2.f);
        blendMode(BlendMode::alpha);
    }

    void drawPaddle()
    {
        const float hw = currentPaddleHW();
        const float pL = paddleX - hw;
        const float pT = PADDLE_Y - PADDLE_H * 0.5f;
        const float pW = hw * 2.f;

        // Glow burst that fades after ball reflection
        if (paddleGlow > 0.01f) {
            blendMode(BlendMode::additive);
            noStroke();
            const int ga = int(paddleGlow * 75.f);
            fill(100, 200, 255, ga);
            rect(pL - 14.f, pT - 14.f, pW + 28.f, PADDLE_H + 28.f, 14.f, 14.f);
            fill(50, 130, 220, ga / 2);
            rect(pL - 26.f, pT - 26.f, pW + 52.f, PADDLE_H + 52.f, 26.f, 26.f);
            blendMode(BlendMode::alpha);
        }

        // Paddle body (green while wide power-up is active, otherwise blue)
        noStroke();
        fill(wideTimer > 0.f ? color(70, 255, 150) : color(60, 145, 255));
        rect(pL, pT, pW, PADDLE_H, 7.f, 7.f);

        // Top highlight strip
        blendMode(BlendMode::additive);
        fill(255, 255, 255, 55);
        rect(pL + 4.f, pT + 2.f, pW - 8.f, 4.f, 2.f, 2.f);
        blendMode(BlendMode::alpha);
    }

    void drawParticles()
    {
        blendMode(BlendMode::additive);
        noStroke();
        for (const auto& p : particles) {
            const float frac = p.life / p.maxLife;
            fill(p5::red(p.col), p5::green(p.col), p5::blue(p.col), int(240.f * frac));
            circle(p.x, p.y, p.size * frac * 2.f);
        }
        blendMode(BlendMode::alpha);
    }

    void drawPowerUps()
    {
        for (const auto& pu : powerUps) {
            if (!pu.alive) continue;
            const float pulse = 0.5f + 0.5f * std::sin(t * 4.5f);
            color_t     c; const char* label;
            switch (pu.type) {
                case PUType::MultiBall: c = color(255,  80, 200); label = "x2"; break;
                case PUType::Wide:      c = color( 80, 255, 100); label = "WD"; break;
                case PUType::Slow:      c = color(100, 200, 255); label = "SL"; break;
            }
            const int cr = p5::red(c), cg = p5::green(c), cb = p5::blue(c);

            // Outer glow
            blendMode(BlendMode::additive);
            noStroke();
            fill(cr, cg, cb, int(65.f * pulse));
            circle(pu.x, pu.y, 28.f);

            // Capsule body
            blendMode(BlendMode::alpha);
            noStroke();
            fill(cr, cg, cb, 230);
            rect(pu.x - 14.f, pu.y - 9.f, 28.f, 18.f, 9.f, 9.f);

            // Label
            noStroke();
            fill(20, 20, 20);
            textAlign(HorizontalTextAlign::center, VerticalTextAlign::center);
            textSize(10.f);
            text(label, pu.x, pu.y);
        }
    }

    void drawHUD()
    {
        noStroke();
        fill(255, 255, 255, 225);
        textAlign(HorizontalTextAlign::left, VerticalTextAlign::top);
        textSize(20.f);
        text("SCORE " + std::to_string(score), 12.f, 10.f);

        fill(255, 85, 85, 225);
        textAlign(HorizontalTextAlign::right, VerticalTextAlign::top);
        text("LIVES " + std::to_string(lives), float(W) - 12.f, 10.f);

        if (combo >= 3) {
            blendMode(BlendMode::additive);
            const float p = 0.5f + 0.5f * std::sin(t * 6.f);
            fill(255, int(220.f * p), 0, 220);
            textAlign(HorizontalTextAlign::center, VerticalTextAlign::top);
            textSize(16.f);
            text("COMBO x" + std::to_string(combo), float(W) * 0.5f, 10.f);
            blendMode(BlendMode::alpha);
        }

        float ty = float(H) - 12.f;
        if (wideTimer > 0.f) {
            fill(80, 255, 100, 200);
            textAlign(HorizontalTextAlign::center, VerticalTextAlign::bottom);
            textSize(12.f);
            text("WIDE  " + std::to_string(int(wideTimer) + 1) + "s", float(W) * 0.5f, ty);
            ty -= 16.f;
        }
        if (slowTimer > 0.f) {
            fill(100, 200, 255, 200);
            textAlign(HorizontalTextAlign::center, VerticalTextAlign::bottom);
            textSize(12.f);
            text("SLOW  " + std::to_string(int(slowTimer) + 1) + "s", float(W) * 0.5f, ty);
        }
    }

    void drawStartScreen()
    {
        const float pulse = 0.5f + 0.5f * std::sin(t * 2.2f);

        // Title — additive blue bloom behind it
        blendMode(BlendMode::additive);
        noStroke();
        fill(0, 90, 255, int(80.f * pulse));
        textAlign(HorizontalTextAlign::center, VerticalTextAlign::center);
        textSize(72.f);
        text("BREAKOUT", float(W) * 0.5f + 4.f, float(H) * 0.5f - 100.f + 4.f);

        blendMode(BlendMode::alpha);
        fill(85, 195, 255);
        textSize(72.f);
        text("BREAKOUT", float(W) * 0.5f, float(H) * 0.5f - 100.f);

        fill(200, 220, 255, int(190.f + 65.f * pulse));
        textSize(18.f);
        text("Move mouse to control the paddle", float(W) * 0.5f, float(H) * 0.5f + 20.f);

        fill(165, 205, 255, int(140.f + 115.f * pulse));
        textSize(16.f);
        text("Click to Start", float(W) * 0.5f, float(H) * 0.5f + 58.f);

        fill(160, 175, 200, 160);
        textSize(11.f);
        text("x2 = Multi Ball   |   WD = Wide Paddle   |   SL = Slow Ball",
             float(W) * 0.5f, float(H) * 0.5f + 100.f);
    }

    void drawEndScreen(const char* title, color_t titleCol)
    {
        // Semi-transparent veil
        noStroke(); fill(0, 0, 0, 155);
        rect(0.f, 0.f, float(W), float(H));

        const float pulse = 0.5f + 0.5f * std::sin(t * 2.5f);
        noStroke(); fill(titleCol);
        textAlign(HorizontalTextAlign::center, VerticalTextAlign::center);
        textSize(58.f);
        text(title, float(W) * 0.5f, float(H) * 0.5f - 75.f);

        fill(255, 255, 255, 220);
        textSize(22.f);
        text("SCORE  " + std::to_string(score), float(W) * 0.5f, float(H) * 0.5f);

        fill(200, 220, 255, int(140.f + 115.f * pulse));
        textSize(16.f);
        text("Click to play again", float(W) * 0.5f, float(H) * 0.5f + 72.f);
    }

    // ── Main draw ────────────────────────────────────────────────────────────
    void draw() override
    {
        t += deltaTime;
        if (state == State::Playing) update(deltaTime);

        // Background is rendered without shake (parallax-like stability)
        drawBackground();

        // Everything else participates in the screen-shake transform
        pushMatrix();
        translate(shakeX, shakeY);

        drawStars();
        drawBricks();
        drawParticles();
        drawPowerUps();

        if (state != State::Start) {
            for (auto& ball : balls) drawBall(ball);
            drawPaddle();
            drawHUD();
        }

        if (state == State::Start)    drawStartScreen();
        if (state == State::GameOver) drawEndScreen("GAME OVER", color(255, 70, 70));
        if (state == State::Win)      drawEndScreen("YOU WIN!",  color(255, 255, 80));

        popMatrix();
    }

    // ── Events ───────────────────────────────────────────────────────────────
    void event(const WindowEvent& e) override
    {
        if (e.type != EventType::mousePress) return;
        if (state == State::Start) {
            state = State::Playing;
        } else if (state == State::GameOver || state == State::Win) {
            initGame();
            state = State::Playing;
        }
    }
};

namespace p5
{
    std::unique_ptr<p5::Sketch> createSketch()
    {
        return std::make_unique<BreakoutSketch>();
    }
} // namespace p5
