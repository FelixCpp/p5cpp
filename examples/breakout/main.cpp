#include <p5cpp.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <numbers>
#include <random>
#include <string>
#include <vector>

using namespace p5cpp;

static constexpr float PI = std::numbers::pi_v<float>;

// ─────────────────────────────────────────────────────────────────────────────
//  BreakoutSketch
//  Goodies:
//    • Animated neon-grid background (GLSL shader + offscreen canvas)
//    • Glowing ball with 24-point motion trail (purple hue during Ghost mode)
//    • Per-brick pulsating colour halos + shiny highlight strip
//    • Crack lines on damaged 2-HP bricks
//    • BOMB bricks – explode all immediate neighbours on destruction (orange ×)
//    • Particle burst explosions (additive blend) + floating score popups
//    • Paddle glow burst on ball reflection
//    • Seven power-up capsules (weighted drop):
//        x2 MultiBall  |  WD Wide Paddle  |  SL Slow Ball  |  LS Laser
//        SH Shield     |  GB Ghost Ball   |  F! Fast (bad)
//    • Laser beams – two neon beams per mouse-click while LS active
//    • Shield line – electric bar near bottom, catches one ball (one-use)
//    • Ghost ball – passes through bricks, instantly destroys all it touches
//    • Screen-shake on brick hit / bomb explosion
//    • Combo multiplier HUD (colour-flashing, additive text)
//    • 3-level progression; each level rebuilds bricks harder & speeds ball up
//    • Level-clear overlay before next level begins
//    • Session high-score displayed on all overlay screens
//    • Scrolling star field behind everything
// ─────────────────────────────────────────────────────────────────────────────

struct BreakoutSketch : p5::Sketch
{
    // ── Window / layout ──────────────────────────────────────────────────────
    static constexpr int W = 800;
    static constexpr int H = 900;
    static constexpr float PADDLE_W = 110.f;
    static constexpr float PADDLE_H = 14.f;
    static constexpr float PADDLE_Y = H - 55.f;
    static constexpr float BALL_R = 8.f;
    static constexpr float BRICK_W = 67.f;
    static constexpr float BRICK_H = 20.f;
    static constexpr float BRICK_GAP = 5.f;
    static constexpr int BRICK_COLS = 10;
    static constexpr int BRICK_ROWS = 7;
    static constexpr float BRICKS_LEFT = 25.f;
    static constexpr float BRICKS_TOP = 90.f;
    static constexpr float BASE_SPEED = 300.f;
    static constexpr int TRAIL_LEN = 24;
    static constexpr float SHIELD_Y = H - 28.f;
    static constexpr int MAX_LEVELS = 3;

    // ── Types ────────────────────────────────────────────────────────────────
    enum class State { Start,
                       Playing,
                       LevelClear,
                       GameOver,
                       Win };

    struct Ball
    {
        float x = 0, y = 0, vx = 0, vy = 0;
        bool alive = true;
        std::array<float2, TRAIL_LEN> trail {};
        int trailLen = 0;
    };

    struct Brick
    {
        float x = 0, y = 0;
        int hp = 1, maxHp = 1;
        bool alive = true;
        bool isBomb = false;
        color_t col = 0;
        float phase = 0.f;
    };

    struct Particle
    {
        float x = 0, y = 0, vx = 0, vy = 0;
        float life = 1.f, maxLife = 1.f, size = 3.f;
        color_t col = 0;
    };

    struct LaserBeam
    {
        float x = 0.f, y = 0.f;
        bool alive = true;
        static constexpr float VY = -680.f;
    };

    struct ScorePopup
    {
        float x = 0.f, y = 0.f;
        float life = 1.3f, maxLife = 1.3f;
        int value = 0;
        color_t col = 0;
    };

    enum class PUType { MultiBall,
                        Wide,
                        Slow,
                        Laser,
                        Shield,
                        Ghost,
                        Fast };

    struct PowerUp
    {
        float x = 0.f, y = 0.f, vy = 115.f;
        PUType type = PUType::MultiBall;
        bool alive = true;
    };

    struct Star
    {
        float x = 0.f, y = 0.f, speed = 0.f, sz = 0.f;
    };

    // ── Persistent state (survives game restarts) ─────────────────────────────
    int highScore = 0;

    // ── Per-game state ────────────────────────────────────────────────────────
    State state = State::Start;
    float paddleX = W / 2.f;
    float paddleGlow = 0.f;
    float wideTimer = 0.f;
    float slowTimer = 0.f;
    float laserTimer = 0.f;
    float ghostTimer = 0.f;
    float shieldHP = 0.f;
    float laserCooldown = 0.f;
    float levelFlashTimer = 0.f;
    int score = 0;
    int lives = 3;
    int combo = 0;
    int level = 1;
    float comboTimer = 0.f;
    float shakeAmt = 0.f;
    float shakeX = 0.f;
    float shakeY = 0.f;
    float t = 0.f;

    std::vector<Ball> balls;
    std::vector<Brick> bricks;
    std::vector<Particle> particles;
    std::vector<PowerUp> powerUps;
    std::vector<LaserBeam> lasers;
    std::vector<ScorePopup> popups;
    std::vector<Star> stars;

    std::shared_ptr<Shader> bgShader;
    std::shared_ptr<Framebuffer> bgCanvas;

    std::mt19937 rng {std::random_device {}()};

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
        const int i = int(h / 60.f) % 6;
        const float f = h / 60.f - int(h / 60.f);
        const float p = v * (1.f - s);
        const float q = v * (1.f - f * s);
        const float u = v * (1.f - (1.f - f) * s);
        switch (i) {
            case 0:
                r = v;
                g = u;
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
                b = u;
                break;
            case 3:
                r = p;
                g = q;
                b = v;
                break;
            case 4:
                r = u;
                g = p;
                b = v;
                break;
            default:
                r = v;
                g = p;
                b = q;
                break;
        }

        return rgba(int(r * 255), int(g * 255), int(b * 255), a);
    }

    float currentPaddleHW() const
    {
        return (wideTimer > 0.f ? PADDLE_W * 1.65f : PADDLE_W) * 0.5f;
    }

    PUType randomPUType()
    {
        const float r = rnd(0.f, 1.f);
        if (r < 0.18f) return PUType::MultiBall;
        else if (r < 0.34f) return PUType::Wide;
        else if (r < 0.50f) return PUType::Slow;
        else if (r < 0.63f) return PUType::Laser;
        else if (r < 0.75f) return PUType::Shield;
        else if (r < 0.87f) return PUType::Ghost;
        else return PUType::Fast;
    }

    void addPopup(float x, float y, int val, color_t col)
    {
        ScorePopup p;
        p.x = x;
        p.y = y;
        p.value = val;
        p.col = col;
        popups.push_back(p);
    }

    // ── setup ────────────────────────────────────────────────────────────────
    void setup() override
    {
        setWindowSize(W, H);
        setWindowTitle("BREAKOUT  ─  p5 Edition");
        setWindowResizable(false);
        frameRate(60);
        buildBgShader();
        bgCanvas = std::shared_ptr<Framebuffer>(createFramebuffer(W, H).release());
        initStars();
        initGame();
    }

    void destroy() override {}

    void buildBgShader()
    {
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

        static constexpr const char* FRAG = R"(
            #version 410 core
            layout(location=0) out vec4 o_Color;
            uniform float uTime;
            uniform vec2  uResolution;

            void main() {
                vec2 uv = gl_FragCoord.xy / uResolution;
                uv.y    = 1.0 - uv.y;

                vec3 col = mix(vec3(0.01, 0.02, 0.08), vec3(0.03, 0.06, 0.14), uv.y);

                vec2  cell  = fract(uv * vec2(14.0, 17.0));
                float lx    = smoothstep(0.0, 0.06, cell.x) * smoothstep(1.0, 0.94, cell.x);
                float ly    = smoothstep(0.0, 0.06, cell.y) * smoothstep(1.0, 0.94, cell.y);
                float grid  = 1.0 - lx * ly;
                float pulse = 0.5 + 0.5 * sin(uTime * 1.4);
                vec3  gc    = mix(vec3(0.0, 0.18, 0.55), vec3(0.0, 0.48, 1.0), pulse);
                col         = mix(col, gc, grid * (0.14 + 0.08 * pulse));

                float hd = abs(uv.y - 0.38);
                col += vec3(0.0, 0.25, 0.7) * exp(-hd * 14.0) * (0.04 + 0.02 * pulse);

                vec2  vig = uv * 2.0 - 1.0;
                col *= 1.0 - dot(vig, vig) * 0.30;

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
            s.x = rnd(0.f, float(W));
            s.y = rnd(0.f, float(H));
            s.speed = rnd(8.f, 55.f);
            s.sz = rnd(0.4f, 2.6f);
        }
    }

    // ── Game lifecycle ───────────────────────────────────────────────────────
    void initGame()
    {
        score = 0;
        lives = 3;
        combo = 0;
        level = 1;
        comboTimer = 0.f;
        shakeAmt = 0.f;
        wideTimer = 0.f;
        slowTimer = 0.f;
        laserTimer = 0.f;
        ghostTimer = 0.f;
        shieldHP = 0.f;
        laserCooldown = 0.f;
        levelFlashTimer = 0.f;
        paddleGlow = 0.f;
        paddleX = W / 2.f;
        balls.clear();
        particles.clear();
        powerUps.clear();
        lasers.clear();
        popups.clear();
        spawnBall();
        buildBricks();
    }

    void nextLevel()
    {
        level++;
        powerUps.clear();
        lasers.clear();
        // Speed all surviving balls up by 10 %
        for (auto& b : balls) {
            const float sp = std::sqrt(b.vx * b.vx + b.vy * b.vy);
            const float fac = (sp * 1.10f) / sp;
            b.vx *= fac;
            b.vy *= fac;
        }
        buildBricks();
    }

    void spawnBall(float fromX = float(W) / 2.f, float fromY = PADDLE_Y - BALL_R - 4.f, float extraAngle = 0.f)
    {
        Ball b;
        b.x = fromX;
        b.y = fromY;
        const float a = -PI / 2.f + extraAngle + rnd(-0.4f, 0.4f);
        b.vx = BASE_SPEED * std::cos(a);
        b.vy = BASE_SPEED * std::sin(a);
        balls.push_back(b);
    }

    void buildBricks()
    {
        bricks.clear();
        // Level N → rows 0..(N-1) get 2 HP; bomb chance scales with level
        const int hardRows = level;
        const float bombChance = 0.08f + level * 0.03f;
        for (int row = 0; row < BRICK_ROWS; ++row) {
            for (int col = 0; col < BRICK_COLS; ++col) {
                Brick br;
                br.x = BRICKS_LEFT + col * (BRICK_W + BRICK_GAP);
                br.y = BRICKS_TOP + row * (BRICK_H + BRICK_GAP);
                br.maxHp = br.hp = (row < hardRows) ? 2 : 1;
                br.col = hsv(float(row) * (360.f / BRICK_ROWS) + 10.f, 0.85f, 1.f);
                br.phase = rnd(0.f, 2.f * PI);
                br.isBomb = (rnd(0.f, 1.f) < bombChance);
                if (br.isBomb)
                    br.col = hsv(rnd(20.f, 45.f), 0.92f, 1.f); // orange hue
                bricks.push_back(br);
            }
        }
    }

    // ── Explosion helpers ────────────────────────────────────────────────────
    void spawnBurst(float x, float y, color_t c, int n, bool big)
    {
        for (int i = 0; i < n; ++i) {
            Particle p;
            p.x = x + rnd(-BRICK_W * 0.35f, BRICK_W * 0.35f);
            p.y = y + rnd(-BRICK_H * 0.5f, BRICK_H * 0.5f);
            const float a = rnd(0.f, 2.f * PI);
            const float sp = big ? rnd(70.f, 330.f) : rnd(25.f, 120.f);
            p.vx = sp * std::cos(a);
            p.vy = sp * std::sin(a) - (big ? 90.f : 0.f);
            p.life = p.maxLife = rnd(0.25f, big ? 1.1f : 0.55f);
            p.size = big ? rnd(2.f, 7.f) : rnd(1.f, 3.5f);
            p.col = c;
            particles.push_back(p);
        }
    }

    void explodeBomb(float bx, float by)
    {
        // Big orange blast at bomb centre
        shakeAmt = std::min(shakeAmt + 0.75f, 2.f);
        spawnBurst(bx + BRICK_W * 0.5f, by + BRICK_H * 0.5f, rgba(255, 140, 0), 40, true);

        // Destroy all alive bricks within a 1-cell radius
        const float cx = bx + BRICK_W * 0.5f;
        const float cy = by + BRICK_H * 0.5f;
        for (auto& nb : bricks) {
            if (!nb.alive) continue;
            const float nx = nb.x + BRICK_W * 0.5f;
            const float ny = nb.y + BRICK_H * 0.5f;
            if (std::abs(nx - cx) <= BRICK_W + BRICK_GAP + 2.f &&
                std::abs(ny - cy) <= BRICK_H + BRICK_GAP + 2.f) {
                nb.alive = false;
                score += 15;
                spawnBurst(nx, ny, nb.col, 16, true);
                addPopup(nx, nb.y, 15, nb.col);
            }
        }
    }

    // ── Power-up application ─────────────────────────────────────────────────
    void applyPU(PUType type)
    {
        switch (type) {
            case PUType::MultiBall: {
                std::vector<Ball> extras;
                for (auto& b : balls) {
                    Ball nb = b;
                    nb.trail = {};
                    nb.trailLen = 0;
                    const float sp = std::sqrt(nb.vx * nb.vx + nb.vy * nb.vy);
                    const float a = std::atan2(nb.vy, nb.vx) + PI / 5.f;
                    nb.vx = sp * std::cos(a);
                    nb.vy = sp * std::sin(a);
                    extras.push_back(nb);
                }
                for (auto& e : extras) balls.push_back(e);
                break;
            }
            case PUType::Wide: wideTimer = 9.f; break;
            case PUType::Slow: slowTimer = 6.f; break;
            case PUType::Laser: laserTimer = 8.f; break;
            case PUType::Shield: shieldHP = 1.f; break;
            case PUType::Ghost: ghostTimer = 5.f; break;
            case PUType::Fast: {
                // Bad: speed all balls up by 38 %, cap at 2.5× BASE_SPEED
                for (auto& b : balls) {
                    const float sp = std::sqrt(b.vx * b.vx + b.vy * b.vy);
                    const float newSp = std::min(sp * 1.38f, BASE_SPEED * 2.5f);
                    const float fac = newSp / sp;
                    b.vx *= fac;
                    b.vy *= fac;
                }
                break;
            }
        }
    }

    void fireLasers()
    {
        const float hw = currentPaddleHW();
        const float pT = PADDLE_Y - PADDLE_H * 0.5f;
        LaserBeam l1, l2;
        l1.x = paddleX - hw * 0.5f;
        l1.y = pT;
        l2.x = paddleX + hw * 0.5f;
        l2.y = pT;
        lasers.push_back(l1);
        lasers.push_back(l2);
        laserCooldown = 0.32f;
        spawnBurst(paddleX, pT, rgba(255, 160, 30), 7, false);
    }

    // ── Update ───────────────────────────────────────────────────────────────
    void updateParticlesAndStars(float dt)
    {
        for (auto& p : particles) {
            p.x += p.vx * dt;
            p.y += p.vy * dt;
            p.vy += 240.f * dt;
            p.life -= dt;
        }
        particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) {
                            return p.life <= 0.f;
                        }),
                        particles.end());
        for (auto& pp : popups) {
            pp.y -= 45.f * dt;
            pp.life -= dt;
        }
        popups.erase(std::remove_if(popups.begin(), popups.end(), [](const ScorePopup& p) {
                         return p.life <= 0.f;
                     }),
                     popups.end());
        for (auto& s : stars) {
            s.y += s.speed * dt;
            if (s.y > float(H) + 4.f) {
                s.y = -4.f;
                s.x = rnd(0.f, float(W));
            }
        }
    }

    void update(float dt)
    {
        // ── Level-clear pause ────────────────────────────────────────────────
        if (state == State::LevelClear) {
            levelFlashTimer -= dt;
            if (levelFlashTimer <= 0.f) {
                nextLevel();
                state = State::Playing;
            }
            updateParticlesAndStars(dt);
            return;
        }

        comboTimer -= dt;
        if (comboTimer <= 0.f) combo = 0;
        wideTimer = std::max(0.f, wideTimer - dt);
        slowTimer = std::max(0.f, slowTimer - dt);
        laserTimer = std::max(0.f, laserTimer - dt);
        ghostTimer = std::max(0.f, ghostTimer - dt);
        laserCooldown = std::max(0.f, laserCooldown - dt);
        paddleGlow = std::max(0.f, paddleGlow - dt * 3.f);

        // Paddle follows mouse
        const float pHW = currentPaddleHW();
        paddleX = std::clamp(float(mouseX), pHW, float(W) - pHW);

        // Screen-shake decay
        if (shakeAmt > 0.f) {
            shakeAmt -= dt * 7.f;
            shakeX = rnd(-shakeAmt * 7.f, shakeAmt * 7.f);
            shakeY = rnd(-shakeAmt * 7.f, shakeAmt * 7.f);
        } else {
            shakeX = shakeY = 0.f;
        }

        const float sm = slowTimer > 0.f ? 0.55f : 1.f;

        // ── Ball update ──────────────────────────────────────────────────────
        for (auto& ball : balls) {
            if (!ball.alive) continue;
            ball.x += ball.vx * dt * sm;
            ball.y += ball.vy * dt * sm;

            for (int i = std::min(ball.trailLen, TRAIL_LEN - 1); i > 0; --i)
                ball.trail[i] = ball.trail[i - 1];
            ball.trail[0] = {ball.x, ball.y};
            if (ball.trailLen < TRAIL_LEN) ++ball.trailLen;

            // Wall bounce
            if (ball.x - BALL_R < 0.f) {
                ball.x = BALL_R;
                ball.vx = std::abs(ball.vx);
            }
            if (ball.x + BALL_R > float(W)) {
                ball.x = float(W) - BALL_R;
                ball.vx = -std::abs(ball.vx);
            }
            if (ball.y - BALL_R < 0.f) {
                ball.y = BALL_R;
                ball.vy = std::abs(ball.vy);
            }

            // Shield catch
            if (shieldHP > 0.f && ball.vy > 0.f && ball.y + BALL_R >= SHIELD_Y && ball.y + BALL_R <= SHIELD_Y + 22.f) {
                ball.vy = -std::abs(ball.vy);
                ball.y = SHIELD_Y - BALL_R - 0.5f;
                shieldHP = 0.f;
                shakeAmt = std::min(shakeAmt + 0.35f, 1.f);
                spawnBurst(ball.x, SHIELD_Y, rgba(0, 220, 255), 16, false);
            }

            // Paddle collision
            const float pL = paddleX - pHW, pR = paddleX + pHW;
            const float pT = PADDLE_Y - PADDLE_H * 0.5f;
            if (ball.vy > 0.f && ball.y + BALL_R >= pT && ball.y - BALL_R <= pT + PADDLE_H + 5.f && ball.x >= pL - 2.f && ball.x <= pR + 2.f) {
                const float hit = (ball.x - paddleX) / pHW;
                const float sp = std::sqrt(ball.vx * ball.vx + ball.vy * ball.vy);
                const float an = hit * PI / 2.8f;
                ball.vx = sp * std::sin(an);
                ball.vy = -sp * std::cos(an);
                ball.y = pT - BALL_R - 0.5f;
                paddleGlow = 1.f;
                spawnBurst(ball.x, pT, rgba(120, 210, 255), 9, false);
            }

            // ── Brick collisions ─────────────────────────────────────────────
            if (ghostTimer > 0.f) {
                // Ghost mode: pass through, instantly destroy every overlapping brick
                for (auto& br : bricks) {
                    if (!br.alive) continue;
                    if (ball.x + BALL_R <= br.x || ball.x - BALL_R >= br.x + BRICK_W) continue;
                    if (ball.y + BALL_R <= br.y || ball.y - BALL_R >= br.y + BRICK_H) continue;

                    ++combo;
                    comboTimer = 2.5f;
                    const int pts = 10 * std::max(1, combo / 4);
                    score += pts;
                    const float bx = br.x, by = br.y;
                    const color_t bc = br.col;
                    const bool wasBomb = br.isBomb;
                    br.alive = false;
                    spawnBurst(bx + BRICK_W * 0.5f, by + BRICK_H * 0.5f, bc, 22, true);
                    addPopup(bx + BRICK_W * 0.5f, by, pts, bc);
                    if (wasBomb) explodeBomb(bx, by);
                    if (rnd(0.f, 1.f) < 0.14f) {
                        PowerUp pu;
                        pu.x = bx + BRICK_W * 0.5f;
                        pu.y = by + BRICK_H;
                        pu.type = randomPUType();
                        powerUps.push_back(pu);
                    }
                }
            } else {
                // Normal: SAT AABB, smallest-overlap axis
                for (auto& br : bricks) {
                    if (!br.alive) continue;
                    const float bL = br.x, bR = br.x + BRICK_W;
                    const float bT = br.y, bB = br.y + BRICK_H;
                    const float oL = ball.x + BALL_R - bL;
                    const float oR = bR - (ball.x - BALL_R);
                    const float oT = ball.y + BALL_R - bT;
                    const float oB = bB - (ball.y - BALL_R);
                    if (oL < 0 || oR < 0 || oT < 0 || oB < 0) continue;

                    const float mo = std::min({oL, oR, oT, oB});
                    if (mo == oL) {
                        ball.vx = -std::abs(ball.vx);
                        ball.x = bL - BALL_R;
                    } else if (mo == oR) {
                        ball.vx = std::abs(ball.vx);
                        ball.x = bR + BALL_R;
                    } else if (mo == oT) {
                        ball.vy = -std::abs(ball.vy);
                        ball.y = bT - BALL_R;
                    } else {
                        ball.vy = std::abs(ball.vy);
                        ball.y = bB + BALL_R;
                    }

                    --br.hp;
                    ++combo;
                    comboTimer = 2.5f;
                    const int pts = 10 * std::max(1, combo / 4);
                    score += pts;
                    shakeAmt = std::min(shakeAmt + 0.22f, 1.f);

                    const bool destroyed = br.hp <= 0;
                    const bool wasBomb = br.isBomb;
                    const float bx = br.x, by = br.y;
                    const color_t bc = br.col;

                    spawnBurst(bx + BRICK_W * 0.5f, by + BRICK_H * 0.5f, bc, destroyed ? 22 : 7, destroyed);
                    if (destroyed) {
                        br.alive = false;
                        addPopup(bx + BRICK_W * 0.5f, by, pts, bc);
                        if (wasBomb) explodeBomb(bx, by);
                        if (rnd(0.f, 1.f) < 0.18f) {
                            PowerUp pu;
                            pu.x = bx + BRICK_W * 0.5f;
                            pu.y = by + BRICK_H;
                            pu.type = randomPUType();
                            powerUps.push_back(pu);
                        }
                    }
                    break; // one bounce per ball per frame
                }
            }

            if (ball.y - BALL_R > float(H)) ball.alive = false;
        }

        balls.erase(std::remove_if(balls.begin(), balls.end(), [](const Ball& b) {
                        return !b.alive;
                    }),
                    balls.end());

        if (balls.empty()) {
            if (--lives <= 0) {
                highScore = std::max(highScore, score);
                state = State::GameOver;
                return;
            }
            spawnBall();
        }

        bool anyAlive = false;
        for (const auto& br : bricks)
            if (br.alive) {
                anyAlive = true;
                break;
            }
        if (!anyAlive) {
            if (level >= MAX_LEVELS) {
                highScore = std::max(highScore, score);
                state = State::Win;
            } else {
                state = State::LevelClear;
                levelFlashTimer = 2.5f;
            }
            return;
        }

        // ── Laser beams ──────────────────────────────────────────────────────
        for (auto& l : lasers) {
            if (!l.alive) continue;
            l.y += LaserBeam::VY * dt;
            if (l.y < -20.f) {
                l.alive = false;
                continue;
            }
            for (auto& br : bricks) {
                if (!br.alive) continue;
                if (l.x < br.x - 2.f || l.x > br.x + BRICK_W + 2.f) continue;
                if (l.y > br.y + BRICK_H || l.y + 22.f < br.y) continue;

                --br.hp;
                score += 10;
                const bool destroyed = br.hp <= 0;
                spawnBurst(br.x + BRICK_W * 0.5f, br.y + BRICK_H * 0.5f, br.col, destroyed ? 18 : 5, destroyed);
                if (destroyed) {
                    const float bx = br.x, by = br.y;
                    const bool wb = br.isBomb;
                    const color_t bc = br.col;
                    br.alive = false;
                    addPopup(bx + BRICK_W * 0.5f, by, 10, bc);
                    if (wb) explodeBomb(bx, by);
                }
                l.alive = false;
                break;
            }
        }
        lasers.erase(std::remove_if(lasers.begin(), lasers.end(), [](const LaserBeam& l) {
                         return !l.alive;
                     }),
                     lasers.end());

        // ── Power-ups ────────────────────────────────────────────────────────
        for (auto& pu : powerUps) {
            if (!pu.alive) continue;
            pu.y += pu.vy * dt;
            const float pHalfW = currentPaddleHW();
            if (std::abs(pu.x - paddleX) < pHalfW + 12.f && std::abs(pu.y - PADDLE_Y) < PADDLE_H + 12.f) {
                applyPU(pu.type);
                pu.alive = false;
            }
            if (pu.y > float(H) + 20.f) pu.alive = false;
        }
        powerUps.erase(std::remove_if(powerUps.begin(), powerUps.end(), [](const PowerUp& p) {
                           return !p.alive;
                       }),
                       powerUps.end());

        updateParticlesAndStars(dt);
    }

    // ── Draw helpers ─────────────────────────────────────────────────────────
    void drawBackground()
    {
        pushCanvas(bgCanvas);
        shader(bgShader);
        setUniform("uTime", uniform(t));
        setUniform("uResolution", uniform(float(W), float(H)));
        noStroke();
        fill(255);
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
            const float flicker = 0.55f + 0.45f * std::sin(t * s.speed * 0.25f + s.x * 0.1f);
            fill(210, 225, 255, int(165.f * flicker));
            circle(s.x, s.y, s.sz * 2.f);
        }
        blendMode(BlendMode::alpha);
    }

    void drawBricks()
    {
        for (const auto& br : bricks) {
            if (!br.alive) continue;
            const int r = p5::red(br.col);
            const int g = p5::green(br.col);
            const int b = p5::blue(br.col);
            const float pu = 0.5f + 0.5f * std::sin(t * 1.8f + br.phase);
            const int dm = (br.hp < br.maxHp) ? 130 : 255;

            // Additive glow halo (bomb bricks pulse faster & larger)
            blendMode(BlendMode::additive);
            noStroke();
            if (br.isBomb) {
                const float bp = 0.5f + 0.5f * std::sin(t * 4.5f + br.phase);
                fill(r, g, b, int(45.f * bp + 20.f));
                rect(br.x - 9.f, br.y - 9.f, BRICK_W + 18.f, BRICK_H + 18.f, 10.f, 10.f);
            } else {
                fill(r, g, b, int(25.f * pu + 12.f));
                rect(br.x - 6.f, br.y - 6.f, BRICK_W + 12.f, BRICK_H + 12.f, 8.f, 8.f);
            }
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

            // Bomb × symbol
            if (br.isBomb) {
                noFill();
                stroke(255, 240, 80, 200);
                strokeWeight(1.8f);
                const float cx = br.x + BRICK_W * 0.5f;
                const float cy = br.y + BRICK_H * 0.5f;
                const float rs = 5.f;
                line(cx - rs, cy - rs, cx + rs, cy + rs);
                line(cx + rs, cy - rs, cx - rs, cy + rs);
            }

            // Zig-zag crack on 2-HP bricks that have been hit once
            if (br.maxHp == 2 && br.hp == 1) {
                noFill();
                stroke(0, 0, 0, 130);
                strokeWeight(1.5f);
                const float cx = br.x + BRICK_W * 0.5f;
                line(cx - 7.f, br.y + 2.f, cx, br.y + 8.f);
                line(cx, br.y + 8.f, cx - 4.f, br.y + 13.f);
                line(cx - 4.f, br.y + 13.f, cx + 5.f, br.y + BRICK_H - 2.f);
            }
        }
    }

    void drawBall(const Ball& ball)
    {
        const bool isGhost = ghostTimer > 0.f;

        blendMode(BlendMode::additive);
        noStroke();

        // Trail
        for (int i = ball.trailLen - 1; i >= 0; --i) {
            const float frac = 1.f - float(i) / float(ball.trailLen);
            const float tr = BALL_R * frac * 0.8f;
            if (isGhost)
                fill(180, 40, 255, int(160.f * frac * frac));
            else
                fill(50, 170, 255, int(160.f * frac * frac));
            circle(ball.trail[i].x, ball.trail[i].y, tr * 2.f);
        }

        // Ghost aura ring
        if (isGhost) {
            const float ga = 0.5f + 0.5f * std::sin(t * 8.f);
            fill(180, 0, 255, int(50.f * ga));
            circle(ball.x, ball.y, (BALL_R + 14.f) * 2.f);
            fill(220, 100, 255, int(35.f * ga));
            circle(ball.x, ball.y, (BALL_R + 24.f) * 2.f);
        } else {
            for (int k = 4; k >= 1; --k) {
                fill(80, 160, 255, 35 / k);
                circle(ball.x, ball.y, (BALL_R + float(k) * 4.5f) * 2.f);
            }
        }

        // Opaque core
        blendMode(BlendMode::alpha);
        noStroke();
        if (isGhost)
            fill(230, 160, 255);
        else
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

        // Laser active: bright orange fringe on the top edge
        if (laserTimer > 0.f) {
            blendMode(BlendMode::additive);
            noStroke();
            const float lp = 0.5f + 0.5f * std::sin(t * 10.f);
            fill(255, 120, 0, int(70.f * lp));
            rect(pL - 4.f, pT - 10.f, pW + 8.f, 14.f, 6.f, 6.f);
        }

        // Glow burst after ball reflection
        if (paddleGlow > 0.01f) {
            blendMode(BlendMode::additive);
            noStroke();
            const int ga = int(paddleGlow * 75.f);
            fill(100, 200, 255, ga);
            rect(pL - 14.f, pT - 14.f, pW + 28.f, PADDLE_H + 28.f, 14.f, 14.f);
            fill(50, 130, 220, ga / 2);
            rect(pL - 26.f, pT - 26.f, pW + 52.f, PADDLE_H + 52.f, 26.f, 26.f);
        }
        blendMode(BlendMode::alpha);

        // Paddle body colour depends on active power-ups
        noStroke();
        if (wideTimer > 0.f) fill(rgba(70, 255, 150));
        else if (ghostTimer > 0.f) fill(rgba(200, 80, 255));
        else fill(rgba(60, 145, 255));
        rect(pL, pT, pW, PADDLE_H, 7.f, 7.f);

        // Top highlight strip
        blendMode(BlendMode::additive);
        fill(255, 255, 255, 55);
        rect(pL + 4.f, pT + 2.f, pW - 8.f, 4.f, 2.f, 2.f);
        blendMode(BlendMode::alpha);
    }

    void drawShield()
    {
        if (shieldHP <= 0.f) return;
        const float pulse = 0.5f + 0.5f * std::sin(t * 9.f);
        blendMode(BlendMode::additive);
        noStroke();
        fill(0, 200, 255, int(55.f * pulse));
        rect(0.f, SHIELD_Y - 10.f, float(W), 24.f);
        fill(120, 240, 255, int(130.f + 80.f * pulse));
        rect(0.f, SHIELD_Y - 2.f, float(W), 5.f);
        blendMode(BlendMode::alpha);
    }

    void drawLasers()
    {
        blendMode(BlendMode::additive);
        noStroke();
        for (const auto& l : lasers) {
            if (!l.alive) continue;
            // Wide glow
            fill(255, 90, 0, 50);
            rect(l.x - 7.f, l.y - 20.f, 14.f, 20.f, 3.f, 3.f);
            // Bright core
            fill(255, 210, 60, 230);
            rect(l.x - 2.f, l.y - 20.f, 4.f, 20.f, 2.f, 2.f);
        }
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

    void drawPopups()
    {
        blendMode(BlendMode::additive);
        noStroke();
        textAlign(TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::center});
        for (const auto& pp : popups) {
            const float a = pp.life / pp.maxLife;
            const float sz = 11.f + 6.f * (1.f - a);
            fill(p5::red(pp.col), p5::green(pp.col), p5::blue(pp.col), int(200.f * a));
            textSize(sz);
            text("+" + std::to_string(pp.value), pp.x, pp.y);
        }
        blendMode(BlendMode::alpha);
    }

    void drawPowerUps()
    {
        for (const auto& pu : powerUps) {
            if (!pu.alive) continue;
            const float pulse = 0.5f + 0.5f * std::sin(t * 4.5f);
            color_t c;
            const char* label;
            bool isBad = false;
            switch (pu.type) {
                case PUType::MultiBall:
                    c = rgba(255, 80, 200);
                    label = "x2";
                    break;
                case PUType::Wide:
                    c = rgba(80, 255, 100);
                    label = "WD";
                    break;
                case PUType::Slow:
                    c = rgba(100, 200, 255);
                    label = "SL";
                    break;
                case PUType::Laser:
                    c = rgba(255, 140, 20);
                    label = "LS";
                    break;
                case PUType::Shield:
                    c = rgba(0, 220, 255);
                    label = "SH";
                    break;
                case PUType::Ghost:
                    c = rgba(190, 60, 255);
                    label = "GB";
                    break;
                case PUType::Fast:
                    c = rgba(255, 40, 40);
                    label = "F!";
                    isBad = true;
                    break;
            }
            const int cr = p5::red(c), cg = p5::green(c), cb = p5::blue(c);

            // Outer glow
            blendMode(BlendMode::additive);
            noStroke();
            fill(cr, cg, cb, int(65.f * pulse));
            circle(pu.x, pu.y, 30.f);

            // "Bad" power-up: red pulsing danger ring
            if (isBad) {
                fill(255, 0, 0, int(40.f * pulse));
                circle(pu.x, pu.y, 40.f);
            }

            // Capsule body
            blendMode(BlendMode::alpha);
            noStroke();
            fill(cr, cg, cb, 230);
            rect(pu.x - 14.f, pu.y - 9.f, 28.f, 18.f, 9.f, 9.f);

            // Label
            fill(20, 20, 20);
            textAlign(TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::center});
            textSize(10.f);
            text(label, pu.x, pu.y);
        }
    }

    void drawHUD()
    {
        noStroke();
        fill(255, 255, 255, 225);
        textAlign(TextAlign {.horizontal = HorizontalTextAlign::left, .vertical = VerticalTextAlign::top});
        textSize(20.f);
        text("SCORE " + std::to_string(score), 12.f, 10.f);

        fill(255, 85, 85, 225);
        textAlign(TextAlign {.horizontal = HorizontalTextAlign::right, .vertical = VerticalTextAlign::top});
        text("LIVES " + std::to_string(lives), float(W) - 12.f, 10.f);

        // Level indicator (centre-top)
        fill(180, 200, 255, 180);
        textAlign(TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::top});
        textSize(14.f);
        text("LVL " + std::to_string(level) + " / " + std::to_string(MAX_LEVELS), float(W) * 0.5f, 10.f);

        // Combo
        if (combo >= 3) {
            blendMode(BlendMode::additive);
            const float p = 0.5f + 0.5f * std::sin(t * 6.f);
            fill(255, int(220.f * p), 0, 220);
            textAlign(TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::top});
            textSize(16.f);
            text("COMBO x" + std::to_string(combo), float(W) * 0.5f, 30.f);
            blendMode(BlendMode::alpha);
        }

        // Active power-up timers (bottom)
        float ty = float(H) - 12.f;
        const struct
        {
            float t;
            const char* label;
            color_t c;
        } timers[] = {
            {wideTimer, "WIDE", rgba(80, 255, 100)},
            {slowTimer, "SLOW", rgba(100, 200, 255)},
            {laserTimer, "LASER", rgba(255, 140, 20)},
            {ghostTimer, "GHOST", rgba(190, 60, 255)},
        };
        for (const auto& tm : timers) {
            if (tm.t <= 0.f) continue;
            fill(tm.c);
            textAlign(TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::bottom});
            textSize(12.f);
            text(std::string(tm.label) + "  " + std::to_string(int(tm.t) + 1) + "s", float(W) * 0.5f, ty);
            ty -= 16.f;
        }

        // Laser click hint
        if (laserTimer > 0.f) {
            const float p = 0.5f + 0.5f * std::sin(t * 6.f);
            blendMode(BlendMode::additive);
            fill(255, 160, 30, int(160.f * p));
            textAlign(TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::bottom});
            textSize(11.f);
            text("[ CLICK TO FIRE ]", float(W) * 0.5f, ty);
            blendMode(BlendMode::alpha);
        }

        // Shield indicator
        if (shieldHP > 0.f) {
            blendMode(BlendMode::additive);
            const float p = 0.5f + 0.5f * std::sin(t * 7.f);
            fill(0, 220, 255, int(150.f * p));
            textAlign(TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::bottom});
            textSize(12.f);
            text("SHIELD  ACTIVE", float(W) * 0.5f, ty - 16.f);
            blendMode(BlendMode::alpha);
        }
    }

    void drawLevelClearOverlay()
    {
        noStroke();
        fill(0, 0, 0, 140);
        rect(0.f, 0.f, float(W), float(H));

        const float pulse = 0.5f + 0.5f * std::sin(t * 3.f);
        blendMode(BlendMode::additive);
        noStroke();
        fill(80, 255, 140, int(60.f * pulse));
        textAlign(TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::center});
        textSize(62.f);
        text("LEVEL CLEAR!", float(W) * 0.5f + 4.f, float(H) * 0.5f - 60.f + 4.f);
        blendMode(BlendMode::alpha);

        fill(100, 255, 160);
        textSize(62.f);
        text("LEVEL CLEAR!", float(W) * 0.5f, float(H) * 0.5f - 60.f);

        fill(200, 240, 210, int(190.f + 60.f * pulse));
        textSize(22.f);
        text("LEVEL  " + std::to_string(level + 1) + "  INCOMING", float(W) * 0.5f, float(H) * 0.5f + 20.f);

        fill(160, 200, 175, 180);
        textSize(14.f);
        text("Bricks are harder  –  Ball is faster", float(W) * 0.5f, float(H) * 0.5f + 62.f);
    }

    void drawStartScreen()
    {
        const float pulse = 0.5f + 0.5f * std::sin(t * 2.2f);

        blendMode(BlendMode::additive);
        noStroke();
        fill(0, 90, 255, int(80.f * pulse));
        textAlign(TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::center});
        textSize(72.f);
        text("BREAKOUT", float(W) * 0.5f + 4.f, float(H) * 0.5f - 115.f + 4.f);
        blendMode(BlendMode::alpha);

        fill(85, 195, 255);
        textSize(72.f);
        text("BREAKOUT", float(W) * 0.5f, float(H) * 0.5f - 115.f);

        fill(200, 220, 255, int(190.f + 65.f * pulse));
        textSize(17.f);
        text("Move mouse to control the paddle", float(W) * 0.5f, float(H) * 0.5f + 5.f);

        fill(165, 205, 255, int(140.f + 115.f * pulse));
        textSize(16.f);
        text("Click to Start", float(W) * 0.5f, float(H) * 0.5f + 42.f);

        fill(160, 175, 200, 155);
        textSize(11.f);
        text("x2 MultiBall  |  WD Wide  |  SL Slow  |  LS Laser  |  SH Shield  |  GB Ghost  |  F! Fast(BAD)", float(W) * 0.5f, float(H) * 0.5f + 82.f);
        text("BOMB bricks (orange \xc3\x97) explode their neighbours!", float(W) * 0.5f, float(H) * 0.5f + 100.f);

        if (highScore > 0) {
            fill(255, 230, 80, 200);
            textSize(13.f);
            text("BEST  " + std::to_string(highScore), float(W) * 0.5f, float(H) * 0.5f + 125.f);
        }
    }

    void drawEndScreen(const char* title, color_t titleCol)
    {
        noStroke();
        fill(0, 0, 0, 155);
        rect(0.f, 0.f, float(W), float(H));

        const float pulse = 0.5f + 0.5f * std::sin(t * 2.5f);
        noStroke();
        fill(titleCol);
        textAlign(TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::center});
        textSize(58.f);
        text(title, float(W) * 0.5f, float(H) * 0.5f - 90.f);

        fill(255, 255, 255, 220);
        textSize(22.f);
        text("SCORE  " + std::to_string(score), float(W) * 0.5f, float(H) * 0.5f - 20.f);

        if (score >= highScore && score > 0) {
            blendMode(BlendMode::additive);
            fill(255, 230, 50, int(180.f + 75.f * pulse));
            textSize(15.f);
            text("NEW HIGH SCORE!", float(W) * 0.5f, float(H) * 0.5f + 12.f);
            blendMode(BlendMode::alpha);
        } else if (highScore > 0) {
            fill(200, 200, 180, 175);
            textSize(13.f);
            text("BEST  " + std::to_string(highScore), float(W) * 0.5f, float(H) * 0.5f + 12.f);
        }

        fill(200, 220, 255, int(140.f + 115.f * pulse));
        textSize(16.f);
        text("Click to play again", float(W) * 0.5f, float(H) * 0.5f + 72.f);
    }

    // ── Main draw ────────────────────────────────────────────────────────────
    void draw() override
    {
        t += deltaTime;
        if (state == State::Playing || state == State::LevelClear)
            update(deltaTime);

        drawBackground();

        pushMatrix();
        translate(shakeX, shakeY);

        drawStars();
        drawBricks();
        drawParticles();
        drawPowerUps();
        drawLasers();
        drawPopups();
        drawShield();

        if (state != State::Start) {
            for (auto& ball : balls) drawBall(ball);
            drawPaddle();
            drawHUD();
        }

        if (state == State::Start) drawStartScreen();
        if (state == State::LevelClear) drawLevelClearOverlay();
        if (state == State::GameOver) drawEndScreen("GAME OVER", rgba(255, 70, 70));
        if (state == State::Win) drawEndScreen("YOU WIN!", rgba(255, 255, 80));

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
        } else if (state == State::Playing && laserTimer > 0.f && laserCooldown <= 0.f) {
            fireLasers();
        }
    }
};

namespace p5cpp
{
    std::unique_ptr<p5::Sketch> createSketch()
    {
        return std::make_unique<BreakoutSketch>();
    }
} // namespace p5cpp
