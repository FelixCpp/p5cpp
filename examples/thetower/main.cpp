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
//  The Tower  –  visual showcase edition
//
//  NEW in this version:
//   • Shockwave rings that expand and fade on every enemy death
//   • Enemy movement trails (fading ghost squares)
//   • Square shards that tumble out on enemy death
//   • Three distinct bullet types (Normal / Multi / Speed) each with own colour
//   • Target-aim lines: brief beam from tower to intercept point on each shot
//   • Rotating tactical scanner inside the range circle
//   • Circular HP ring drawn around the tower
//   • Full-screen red vignette flash when the tower takes a hit
//   • Ambient star-field drifting slowly in background
//   • Wave-arrival banner that slides in from above
//   • Speedshot charge flash (tower briefly flares white)
//   • Background shader gains an expanding ring ripple on each shot
//   • Range circle pulses on every shot
//   • Floating-text score pop uses a scale-bounce animation
// =============================================================================

struct TowerSketch : p5::Sketch
{
    // ── Layout ───────────────────────────────────────────────────────────────
    static constexpr int W = 800;
    static constexpr int H = 800;
    static constexpr float CX = W * 0.5f;
    static constexpr float CY = H * 0.5f;
    static constexpr float TOWER_RADIUS = 26.f;
    static constexpr float BULLET_R = 4.f;
    static constexpr float BULLET_SPEED = 460.f;
    static constexpr float SPAWN_RADIUS = 460.f;
    static constexpr float ENEMY_SIZE = 15.f;

    enum class State { Menu,
                       Playing,
                       Shop,
                       GameOver };

    // ── Upgrade system ────────────────────────────────────────────────────────
    enum UpId {
        UP_RANGE = 0,
        UP_FIRERATE,
        UP_MULTISHOT,
        UP_SPEEDSHOT,
        UP_BURSTSIZE,
        UP_DAMAGE,
        UP_MAXHP,
        UP_COINBONUS,
        UP_COUNT = 8
    };

    struct Upgrade
    {
        const char* name;
        const char* effectLine;
        int level = 0, maxLevel = 5;
        std::array<int, 5> costs;
        bool maxed() const { return level >= maxLevel; }
        int nextCost() const { return maxed() ? 0 : costs[level]; }
        bool canBuy(int coins) const { return !maxed() && coins >= nextCost(); }
    };

    // clang-format off
    std::array<Upgrade,UP_COUNT> upgrades {{
        {"Range",      "+45 radius",   0,5,{50, 80,120,180,260}},
        {"Fire Rate",  "-0.10s delay", 0,5,{60, 90,130,190,280}},
        {"Multishot",  "+12% chance",  0,5,{70,100,150,220,320}},
        {"Speedshot",  "+6% chance",   0,5,{70,100,150,220,320}},
        {"Burst Size", "+2 bullets",   0,4,{80,130,200,300,  0}},
        {"Bullet Dmg", "+0.5 damage",  0,4,{55, 90,140,210,  0}},
        {"Max HP",     "+25 HP",       0,5,{80,120,170,240,350}},
        {"Coin Bonus", "+50% coins",   0,4,{90,140,210,310,  0}},
    }};
    // clang-format on

    int shopSel = 0;

    float shootRadius() const { return 185.f + upgrades[UP_RANGE].level * 45.f; }
    float shootCooldown() const { return std::max(0.18f, 0.75f - upgrades[UP_FIRERATE].level * 0.10f); }
    float multiShotChance() const { return 0.15f + upgrades[UP_MULTISHOT].level * 0.12f; }
    float speedShotChance() const { return 0.08f + upgrades[UP_SPEEDSHOT].level * 0.06f; }
    int burstMaxShots() const { return 6 + upgrades[UP_BURSTSIZE].level * 2; }
    float bulletDamage() const { return 1.0f + upgrades[UP_DAMAGE].level * 0.5f; }
    float towerMaxHp() const { return 100.f + upgrades[UP_MAXHP].level * 25.f; }
    float coinMultiplier() const { return 1.0f + upgrades[UP_COINBONUS].level * 0.50f; }

    // ── Simulation types ──────────────────────────────────────────────────────
    enum class BulletType { Normal,
                            Multi,
                            Speed };

    struct Enemy
    {
        float2 pos, vel;
        float hp, maxHp, size;
        float angle = 0.f, flashTimer = 0.f;
        bool alive = true;
        std::array<float2, 8> trail {};
        int trailLen = 0;
    };

    struct Bullet
    {
        float2 pos, vel;
        BulletType type = BulletType::Normal;
        bool alive = true;
        std::array<float2, 12> trail {};
        int trailLen = 0;
    };

    struct Particle
    {
        float2 pos, vel;
        color_t col;
        float life, maxLife, size;
        bool isSquare = false;
        float angle = 0.f, angVel = 0.f;
    };

    struct FloatingText
    {
        float2 pos;
        float life, maxLife;
        std::string text;
        color_t col;
        float startScale = 1.6f; // pops large then settles
    };

    struct ShockwaveRing
    {
        float2 pos;
        float life, maxLife;
        float startR, endR;
        color_t col;
        float radius() const { return startR + (endR - startR) * (1.f - life / maxLife); }
    };

    struct TargetLine
    {
        float2 from, to;
        float life, maxLife;
        color_t col;
    };

    struct AmbientStar
    {
        float2 pos, vel;
        float size, brightness, phase;
    };

    // ── Runtime state ─────────────────────────────────────────────────────────
    State state = State::Menu;
    float towerHp = 100.f;
    float towerPulseT = 0.f;
    float shakeX = 0.f, shakeY = 0.f, shakeTimer = 0.f;

    int coins = 0, highScore = 0;

    std::vector<Enemy> enemies;
    std::vector<Bullet> bullets;
    std::vector<Particle> particles;
    std::vector<FloatingText> floatingTexts;
    std::vector<ShockwaveRing> rings;
    std::vector<TargetLine> targetLines;
    std::vector<AmbientStar> stars;

    float shootTimer = 0.f;
    bool speedShotActive = false;
    float speedShotTimer = 0.f;
    int speedShotShots = 0;
    float speedIndicatorT = 0.f;

    int wave = 1;
    float waveTimer = 0.f;
    float spawnInterval = 1.6f;
    float spawnTimer = 0.f;
    int score = 0;

    // visual FX state
    float scannerAngle = 0.f; // rotating scanner line
    float damageFlashT = 0.f; // red screen edge on tower hit
    float waveBannerT = 0.f;  // wave arrival banner countdown
    std::string waveBannerText;
    float chargeFlashT = 0.f;   // tower flare on speedshot trigger
    float rangePulseT = 0.f;    // range ring pulse on each shot
    float lastShotBgT = -100.f; // for background shader ripple

    float bgTime = 0.f;
    std::shared_ptr<Shader> bgShader;
    std::shared_ptr<Framebuffer> bgCanvas;
    std::mt19937 rng {std::random_device {}()};

    // ── Lifecycle ─────────────────────────────────────────────────────────────
    void setup() override
    {
        setWindowSize(W, H);
        setWindowTitle("The Tower");
        setWindowResizable(false);
        frameRate(60);
        buildBgShader();
        bgCanvas = std::shared_ptr<Framebuffer>(createCanvas(W, H).release());
        initStars();
    }

    void destroy() override {}

    void initStars()
    {
        stars.resize(180);
        std::uniform_real_distribution<float> px(0.f, (float)W);
        std::uniform_real_distribution<float> py(0.f, (float)H);
        std::uniform_real_distribution<float> spd(4.f, 22.f);
        std::uniform_real_distribution<float> ang(0.f, TAU);
        std::uniform_real_distribution<float> sz(0.5f, 2.2f);
        std::uniform_real_distribution<float> br(0.3f, 1.f);
        std::uniform_real_distribution<float> ph(0.f, TAU);
        for (auto& s : stars) {
            s.pos = {px(rng), py(rng)};
            float a = ang(rng), v = spd(rng);
            s.vel = {std::cos(a) * v, std::sin(a) * v};
            s.size = sz(rng);
            s.brightness = br(rng);
            s.phase = ph(rng);
        }
    }

    void event(const WindowEvent& e) override
    {
        if (e.type != EventType::keyPress) return;
        Key k = e.keyEvent.key;
        if (state == State::Menu && k == Key::enter) startGame();
        if (state == State::GameOver && k == Key::enter) startGame();
        if (state == State::Playing) {
            if (k == Key::escape) state = State::Menu;
            else if (k == Key::u) state = State::Shop;
        } else if (state == State::Shop) handleShopInput(k);
    }

    void handleShopInput(Key k)
    {
        if (k == Key::escape || k == Key::u) {
            state = State::Playing;
            return;
        }
        if (k == Key::right) shopSel = (shopSel + 1) % UP_COUNT;
        if (k == Key::left) shopSel = (shopSel + UP_COUNT - 1) % UP_COUNT;
        if (k == Key::down) shopSel = (shopSel + 4) % UP_COUNT;
        if (k == Key::up) shopSel = (shopSel + UP_COUNT - 4) % UP_COUNT;
        if (k == Key::enter) buyUpgrade(shopSel);
    }

    void buyUpgrade(int id)
    {
        auto& up = upgrades[id];
        if (!up.canBuy(coins)) return;
        coins -= up.nextCost();
        up.level++;
        if (id == UP_MAXHP) towerHp = std::min(towerHp + 25.f, towerMaxHp());
        spawnRing({CX, CY}, rgba(100, 255, 180), 30.f, 90.f, 0.6f);
        spawnFloatingText({CX, CY - 30.f}, std::string(up.name) + " upgraded!", rgba(100, 255, 180), 1.2f);
    }

    void startGame()
    {
        towerHp = towerMaxHp();
        enemies.clear();
        bullets.clear();
        particles.clear();
        floatingTexts.clear();
        rings.clear();
        targetLines.clear();
        shootTimer = 0.f;
        speedShotActive = false;
        wave = 1;
        waveTimer = 0.f;
        spawnInterval = 1.6f;
        spawnTimer = 0.f;
        score = 0;
        coins = 0;
        shakeTimer = 0.f;
        damageFlashT = 0.f;
        chargeFlashT = 0.f;
        rangePulseT = 0.f;
        waveBannerT = 0.f;
        lastShotBgT = -100.f;
        state = State::Playing;
    }

    // ── Background shader ─────────────────────────────────────────────────────
    void buildBgShader()
    {
        static constexpr const char* VERT = R"(
            #version 410 core
            layout(location=0) in vec2  a_Position;
            layout(location=1) in vec2  a_TexCoord;
            layout(location=2) in vec4  a_Color;
            layout(location=3) in float a_TexIndex;
            out vec2 v_TexCoord; out vec4 v_Color; out float v_TexIndex;
            uniform mat4 u_ProjectionMatrix;
            void main() {
                gl_Position = u_ProjectionMatrix * vec4(a_Position,0.0,1.0);
                v_TexCoord=a_TexCoord; v_Color=a_Color; v_TexIndex=a_TexIndex;
            }
        )";

        static constexpr const char* FRAG = R"(
            #version 410 core
            layout(location=0) out vec4 o_Color;
            uniform float uTime;
            uniform vec2  uResolution;
            uniform float uLastShotT;   // bgTime at last shot (for ripple)

            void main() {
                vec2 uv  = gl_FragCoord.xy / uResolution;
                uv.y     = 1.0 - uv.y;
                vec2 c   = uv - 0.5;
                float d  = length(c);

                // base gradient
                vec3 col = mix(vec3(0.02,0.02,0.10), vec3(0.00,0.01,0.06), uv.y);

                // grid
                vec2  grid  = fract(uv*18.0);
                float lx    = smoothstep(0.0,0.04,grid.x)*smoothstep(1.0,0.96,grid.x);
                float ly    = smoothstep(0.0,0.04,grid.y)*smoothstep(1.0,0.96,grid.y);
                float pulse = 0.5+0.5*sin(uTime*0.9);
                col = mix(col, vec3(0.0,0.3,0.8)*(0.18+0.10*pulse), (1.0-lx*ly)*0.22);

                // centre glow
                col += vec3(0.05,0.15,0.55)*exp(-d*4.5)*(0.12+0.06*sin(uTime*1.3));

                // shot ripple – expanding ring from centre
                float dt   = uTime - uLastShotT;
                float rRad = dt * 0.55;          // ripple expands outward
                float rAmt = exp(-dt*2.8);        // fades over time
                float rDist= abs(d - rRad);
                col += vec3(0.1,0.4,1.0) * rAmt * smoothstep(0.06,0.0,rDist);

                // scanlines
                float scan = fract(uv.y*200.0-uTime*0.08);
                col *= 1.0-0.06*smoothstep(0.0,0.03,scan)*smoothstep(0.06,0.03,scan);

                // vignette
                col *= 1.0-dot(c*1.6,c*1.6);
                o_Color = vec4(col,1.0);
            }
        )";
        bgShader = std::shared_ptr<Shader>(loadShader(VERT, FRAG).release());
    }

    // ── Main draw / update ────────────────────────────────────────────────────
    void draw() override
    {
        bgTime += deltaTime;
        renderBackground();
        if (state == State::Menu) drawMenu();
        else if (state == State::Playing) {
            update();
            drawGame();
        } else if (state == State::Shop) {
            drawGame();
            drawShop();
        } else {
            drawGame();
            drawGameOver();
        }
    }

    // ── Update ────────────────────────────────────────────────────────────────
    void update()
    {
        const float dt = std::min(deltaTime, 0.05f);
        scannerAngle += dt * (TAU / 3.f); // one full revolution per 3 s

        waveTimer += dt;
        if (waveTimer >= 20.f) {
            waveTimer = 0.f;
            wave++;
            spawnInterval = std::max(0.32f, spawnInterval * 0.82f);
            waveBannerText = "Wave " + std::to_string(wave) + "!";
            waveBannerT = 2.2f;
            spawnRing({CX, CY}, rgba(255, 200, 60), 40.f, 160.f, 0.8f);
        }
        damageFlashT = std::max(0.f, damageFlashT - dt * 3.f);
        chargeFlashT = std::max(0.f, chargeFlashT - dt * 4.f);
        rangePulseT = std::max(0.f, rangePulseT - dt * 4.f);
        waveBannerT = std::max(0.f, waveBannerT - dt);
        speedIndicatorT = std::max(0.f, speedIndicatorT - dt * 1.5f);

        updateStars(dt);
        updateSpawner(dt);
        updateEnemies(dt);
        updateShooting(dt);
        updateBullets(dt);
        updateParticles(dt);
        updateRings(dt);
        updateTargetLines(dt);
        updateShake(dt);

        if (towerHp <= 0.f) {
            towerHp = 0.f;
            if (score > highScore) highScore = score;
            state = State::GameOver;
        }
    }

    // ── Stars ─────────────────────────────────────────────────────────────────
    void updateStars(float dt)
    {
        for (auto& s : stars) {
            s.pos = s.pos + s.vel * dt;
            s.phase += dt * 1.2f;
            if (s.pos.x < -5) s.pos.x = W + 5;
            if (s.pos.x > W + 5) s.pos.x = -5;
            if (s.pos.y < -5) s.pos.y = H + 5;
            if (s.pos.y > H + 5) s.pos.y = -5;
        }
    }

    // ── Spawner ───────────────────────────────────────────────────────────────
    void updateSpawner(float dt)
    {
        spawnTimer -= dt;
        if (spawnTimer > 0.f) return;
        spawnTimer = spawnInterval;
        int count = 1 + (wave - 1) / 3;
        for (int i = 0; i < count; ++i) spawnEnemy();
    }

    void spawnEnemy()
    {
        std::uniform_real_distribution<float> angleDist(0.f, TAU);
        std::uniform_real_distribution<float> speedVar(-12.f, 12.f);
        float angle = angleDist(rng);
        float2 pos = {CX + std::cos(angle) * SPAWN_RADIUS, CY + std::sin(angle) * SPAWN_RADIUS};
        float2 dir = normalized(float2 {CX, CY} - pos);
        float spd = 55.f + wave * 6.f + speedVar(rng);
        Enemy e;
        e.pos = pos;
        e.vel = dir * spd;
        e.hp = e.maxHp = 1.f + wave * 0.25f;
        e.size = ENEMY_SIZE;
        enemies.push_back(e);
    }

    // ── Enemies ───────────────────────────────────────────────────────────────
    void updateEnemies(float dt)
    {
        for (auto& e : enemies) {
            if (!e.alive) continue;
            // trail
            if (e.trailLen < (int)e.trail.size()) {
                e.trail[e.trailLen++] = e.pos;
            } else {
                for (int i = 0; i < (int)e.trail.size() - 1; ++i) e.trail[i] = e.trail[i + 1];
                e.trail.back() = e.pos;
            }
            float2 toTower = normalized(float2 {CX, CY} - e.pos);
            float spd = length(e.vel);
            e.vel = normalized(e.vel + toTower * dt * 80.f) * spd;
            e.pos = e.pos + e.vel * dt;
            e.angle += dt * 2.5f;
            e.flashTimer = std::max(0.f, e.flashTimer - dt * 6.f);
            if (length(e.pos - float2 {CX, CY}) < TOWER_RADIUS + e.size * 0.6f) {
                e.alive = false;
                float dmg = 5.f + wave * 0.4f;
                towerHp -= dmg;
                damageFlashT = 1.f;
                triggerShake(0.3f);
                spawnParticles(e.pos, rgba(255, 80, 60), 14);
                spawnRing(e.pos, rgba(255, 80, 60), 0.f, 50.f, 0.4f);
            }
        }
        std::erase_if(enemies, [](const Enemy& e) {
            return !e.alive;
        });
    }

    // ── Shooting ──────────────────────────────────────────────────────────────
    std::vector<Enemy*> enemiesInRange()
    {
        float r = shootRadius();
        std::vector<Enemy*> result;
        for (auto& e : enemies)
            if (e.alive && length(e.pos - float2 {CX, CY}) <= r)
                result.push_back(&e);
        std::sort(result.begin(), result.end(), [](const Enemy* a, const Enemy* b) {
            return length(a->pos - float2 {CX, CY}) < length(b->pos - float2 {CX, CY});
        });
        return result;
    }

    float2 intercept(float2 tp, float2 tv) const
    {
        float2 d = tp - float2 {CX, CY};
        float a = dot(tv, tv) - BULLET_SPEED * BULLET_SPEED;
        float b = 2.f * dot(d, tv);
        float c = dot(d, d);
        float t = -1.f;
        if (std::abs(a) < 1e-4f) {
            if (std::abs(b) > 1e-4f) t = -c / b;
        } else {
            float disc = b * b - 4.f * a * c;
            if (disc >= 0.f) {
                float sq = std::sqrt(disc);
                float t1 = (-b - sq) / (2.f * a), t2 = (-b + sq) / (2.f * a);
                if (t1 > 0.f) t = t1;
                else if (t2 > 0.f) t = t2;
            }
        }
        float2 aim = (t > 0.f) ? tp + tv * t : tp;
        return normalized(aim - float2 {CX, CY});
    }

    void fireBullet(float2 dir, BulletType type)
    {
        Bullet b;
        b.pos = {CX, CY};
        b.vel = dir * BULLET_SPEED;
        b.type = type;
        bullets.push_back(b);

        // Aim line: from tower to range boundary
        float r = shootRadius();
        TargetLine tl;
        tl.from = {CX, CY};
        tl.to = {CX + dir.x * r, CY + dir.y * r};
        tl.life = tl.maxLife = 0.18f;
        tl.col = (type == BulletType::Multi) ? rgba(80, 220, 255) : (type == BulletType::Speed) ? rgba(255, 160, 40)
                                                                                                : rgba(255, 230, 80);
        targetLines.push_back(tl);

        lastShotBgT = bgTime;
        rangePulseT = 1.f;
    }

    void updateShooting(float dt)
    {
        if (speedShotActive) {
            speedShotTimer -= dt;
            if (speedShotTimer <= 0.f && speedShotShots < burstMaxShots()) {
                auto inRange = enemiesInRange();
                if (!inRange.empty())
                    fireBullet(intercept(inRange.front()->pos, inRange.front()->vel), BulletType::Speed);
                speedShotShots++;
                speedShotTimer = 0.13f;
            }
            if (speedShotShots >= burstMaxShots()) speedShotActive = false;
        }
        shootTimer -= dt;
        if (shootTimer > 0.f) return;
        shootTimer = shootCooldown();
        auto inRange = enemiesInRange();
        if (inRange.empty()) return;

        std::uniform_real_distribution<float> roll(0.f, 1.f);
        if (inRange.size() >= 2 && roll(rng) < multiShotChance()) {
            fireBullet(intercept(inRange[0]->pos, inRange[0]->vel), BulletType::Multi);
            fireBullet(intercept(inRange[1]->pos, inRange[1]->vel), BulletType::Multi);
            spawnFloatingText({CX, CY - 55.f}, "MULTI!", rgba(100, 200, 255), 0.7f);
        } else {
            fireBullet(intercept(inRange.front()->pos, inRange.front()->vel), BulletType::Normal);
        }

        if (!speedShotActive && roll(rng) < speedShotChance()) {
            speedShotActive = true;
            speedShotTimer = 0.f;
            speedShotShots = 0;
            speedIndicatorT = 1.f;
            chargeFlashT = 1.f;
            spawnRing({CX, CY}, rgba(80, 255, 200), 28.f, 80.f, 0.4f);
            spawnFloatingText({CX, CY - 68.f}, "SPEED SHOT!", rgba(80, 255, 200), 0.9f);
        }
    }

    // ── Bullets ───────────────────────────────────────────────────────────────
    void updateBullets(float dt)
    {
        for (auto& b : bullets) {
            if (!b.alive) continue;
            if (b.trailLen < (int)b.trail.size()) {
                b.trail[b.trailLen++] = b.pos;
            } else {
                for (int i = 0; i < (int)b.trail.size() - 1; ++i) b.trail[i] = b.trail[i + 1];
                b.trail.back() = b.pos;
            }
            b.pos = b.pos + b.vel * dt;
            if (b.pos.x < -60 || b.pos.x > W + 60 || b.pos.y < -60 || b.pos.y > H + 60) {
                b.alive = false;
                continue;
            }
            for (auto& e : enemies) {
                if (!e.alive) continue;
                if (length(b.pos - e.pos) < e.size * 0.75f + BULLET_R) {
                    b.alive = false;
                    e.flashTimer = 1.f;
                    e.hp -= bulletDamage();
                    spawnParticles(b.pos, rgba(255, 200, 80), 5);
                    if (e.hp <= 0.f) {
                        e.alive = false;
                        int pts = 10 * wave;
                        score += pts;
                        int drop = (int)std::round((2 + wave) * coinMultiplier());
                        coins += drop;
                        spawnDeathBurst(e.pos);
                        spawnRing(e.pos, rgba(255, 160, 40), 0.f, 65.f, 0.55f);
                        spawnRing(e.pos, rgba(255, 220, 100), 0.f, 40.f, 0.35f);
                        spawnFloatingText(e.pos, "+" + std::to_string(pts) + "  (" + std::to_string(drop) + "c)", rgba(255, 220, 80), 0.95f);
                    }
                    break;
                }
            }
        }
        std::erase_if(bullets, [](const Bullet& b) {
            return !b.alive;
        });
        std::erase_if(enemies, [](const Enemy& e) {
            return !e.alive;
        });
    }

    // ── Particles ─────────────────────────────────────────────────────────────
    void updateParticles(float dt)
    {
        for (auto& p : particles) {
            p.pos = p.pos + p.vel * dt;
            p.vel = p.vel * (1.f - dt * 3.2f);
            p.life -= dt;
            if (p.isSquare) p.angle += p.angVel * dt;
        }
        std::erase_if(particles, [](const Particle& p) {
            return p.life <= 0.f;
        });
        for (auto& ft : floatingTexts) {
            ft.pos.y -= dt * 26.f;
            ft.life -= dt;
        }
        std::erase_if(floatingTexts, [](const FloatingText& ft) {
            return ft.life <= 0.f;
        });
    }

    void updateRings(float dt)
    {
        for (auto& r : rings) r.life -= dt;
        std::erase_if(rings, [](const ShockwaveRing& r) {
            return r.life <= 0.f;
        });
    }

    void updateTargetLines(float dt)
    {
        for (auto& tl : targetLines) tl.life -= dt;
        std::erase_if(targetLines, [](const TargetLine& tl) {
            return tl.life <= 0.f;
        });
    }

    void updateShake(float dt)
    {
        if (shakeTimer > 0.f) {
            shakeTimer -= dt;
            std::uniform_real_distribution<float> sd(-5.f, 5.f);
            shakeX = sd(rng);
            shakeY = sd(rng);
        } else {
            shakeX = shakeY = 0.f;
        }
    }

    void triggerShake(float dur) { shakeTimer = std::max(shakeTimer, dur); }

    void spawnParticles(float2 pos, color_t col, int n)
    {
        std::uniform_real_distribution<float> ang(0.f, TAU);
        std::uniform_real_distribution<float> spd(60.f, 220.f);
        std::uniform_real_distribution<float> sz(2.f, 5.f);
        for (int i = 0; i < n; ++i) {
            float a = ang(rng), s = spd(rng);
            Particle p;
            p.pos = pos;
            p.vel = {std::cos(a) * s, std::sin(a) * s};
            p.col = col;
            p.size = sz(rng);
            p.life = p.maxLife = 0.25f + sz(rng) * 0.04f;
            particles.push_back(p);
        }
    }

    void spawnSquareShard(float2 pos, color_t col)
    {
        std::uniform_real_distribution<float> ang(0.f, TAU);
        std::uniform_real_distribution<float> spd(80.f, 260.f);
        std::uniform_real_distribution<float> av(-8.f, 8.f);
        std::uniform_real_distribution<float> sz(3.f, 7.f);
        float a = ang(rng), s = spd(rng);
        Particle p;
        p.pos = pos;
        p.vel = {std::cos(a) * s, std::sin(a) * s};
        p.col = col;
        p.size = sz(rng);
        p.life = p.maxLife = 0.35f + sz(rng) * 0.04f;
        p.isSquare = true;
        p.angle = ang(rng);
        p.angVel = av(rng);
        particles.push_back(p);
    }

    void spawnDeathBurst(float2 pos)
    {
        spawnParticles(pos, rgba(255, 140, 40), 22);
        for (int i = 0; i < 8; ++i) spawnSquareShard(pos, rgba(255, 80, 40));
        for (int i = 0; i < 3; ++i) spawnSquareShard(pos, rgba(255, 200, 80));
        for (int i = 0; i < 12; ++i) {
            float a = TAU * i / 12.f;
            Particle p;
            p.pos = pos;
            p.vel = {std::cos(a) * 170.f, std::sin(a) * 170.f};
            p.col = rgba(255, 220, 100, 150);
            p.size = 3.f;
            p.life = p.maxLife = 0.45f;
            particles.push_back(p);
        }
    }

    void spawnRing(float2 pos, color_t col, float startR, float endR, float life)
    {
        rings.push_back({pos, life, life, startR, endR, col});
    }

    void spawnFloatingText(float2 pos, const std::string& txt, color_t col, float dur)
    {
        FloatingText ft;
        ft.pos = pos;
        ft.text = txt;
        ft.col = col;
        ft.life = ft.maxLife = dur;
        ft.startScale = 1.6f;
        floatingTexts.push_back(ft);
    }

    // ── Rendering helpers ─────────────────────────────────────────────────────
    void polygon(float cx, float cy, float r, int sides, float rot = 0.f)
    {
        beginShape();
        for (int i = 0; i < sides; ++i) {
            float a = rot + TAU * i / sides;
            vertex(cx + std::cos(a) * r, cy + std::sin(a) * r);
        }
        endShape(ShapeType::polygon);
    }

    void glowPolygon(float cx, float cy, float r, int sides, float rot, int R, int G, int B, int layers = 5, float spread = 20.f)
    {
        noStroke();
        for (int i = layers; i >= 0; --i) {
            float t = (float)i / layers;
            fill(R, G, B, (int)(140.f * (1.f - t) * (1.f - t)));
            polygon(cx, cy, r + t * spread, sides, rot);
        }
    }

    void renderBackground()
    {
        pushCanvas(bgCanvas);
        shader(bgShader);
        setUniform("uTime", uniform(bgTime));
        setUniform("uResolution", uniform((float)W, (float)H));
        setUniform("uLastShotT", uniform(lastShotBgT));
        noStroke();
        fill(255);
        rect(0, 0, (float)W, (float)H);
        noShader();
        popCanvas();
        blendMode(BlendMode::alpha);
        noTint();
        image(bgCanvas->getTextureId(), 0, 0, (float)W, (float)H);
    }

    // ── drawGame ─────────────────────────────────────────────────────────────
    void drawGame()
    {
        // ambient stars (very subtle)
        blendMode(BlendMode::additive);
        noStroke();
        for (const auto& s : stars) {
            float tw = 0.5f + 0.5f * std::sin(bgTime * 1.4f + s.phase);
            int a = (int)(s.brightness * tw * 60.f);
            fill(180, 210, 255, a);
            circle(s.pos.x, s.pos.y, s.size * 2.f);
        }

        // shockwave rings
        noFill();
        noStroke();
        for (const auto& r : rings) {
            float t = r.life / r.maxLife;
            float rad = r.radius();
            stroke(red(r.col), green(r.col), blue(r.col), (int)(alpha(r.col) * t * t));
            strokeWeight(2.f * t + 0.5f);
            circle(r.pos.x, r.pos.y, rad * 2.f);
        }

        // target aim lines
        for (const auto& tl : targetLines) {
            float t = tl.life / tl.maxLife;
            stroke(red(tl.col), green(tl.col), blue(tl.col), (int)(160.f * t));
            strokeWeight(1.f + t);
            line(tl.from.x, tl.from.y, tl.to.x, tl.to.y);
        }

        drawRangeCircle();

        noStroke();
        drawParticles();
        drawEnemyTrails();
        drawBullets();
        drawEnemies();
        drawTower();

        blendMode(BlendMode::alpha);
        drawDamageFlash();
        drawFloatingTexts();
        drawHUD();
        drawWaveBanner();
    }

    // ── Range circle ──────────────────────────────────────────────────────────
    void drawRangeCircle()
    {
        float r = shootRadius();
        float cx = CX + shakeX, cy = CY + shakeY;
        bool hasTarget = !enemiesInRange().empty();
        float pulse = rangePulseT;

        blendMode(BlendMode::additive);
        noStroke();
        fill(hasTarget ? 30 : 15, hasTarget ? 110 : 70, 255, (int)(8.f + pulse * 14.f));
        circle(cx, cy, r * 2.f);

        // scanner line
        noFill();
        strokeWeight(1.5f);
        // scanner fan (trailing glow)
        for (int i = 0; i < 6; ++i) {
            float lag = i / 6.f;
            float sang = scannerAngle - lag * 0.8f;
            int a = (int)((1.f - lag) * 50.f * (hasTarget ? 1.4f : 0.7f));
            stroke(60, 200, 255, a);
            strokeWeight(1.f * (1.f - lag) + 0.3f);
            line(cx, cy, cx + std::cos(sang) * r, cy + std::sin(sang) * r);
        }

        // dashed ring
        static const std::array<float, 2> dashPat {10.f, 7.f};
        strokePattern(dashPat);
        strokeWeight(1.2f + pulse * 1.5f);
        stroke(hasTarget ? 80 : 50, hasTarget ? 200 : 140, 255, (int)((hasTarget ? 90 : 55) + pulse * 60.f));
        circle(cx, cy, r * 2.f);
        static const std::array<float, 0> noPat {};
        strokePattern(noPat);

        blendMode(BlendMode::alpha);
    }

    // ── Tower ─────────────────────────────────────────────────────────────────
    void drawTower()
    {
        towerPulseT += deltaTime;
        float pulse = 1.f + 0.03f * std::sin(towerPulseT * 3.f);
        float charge = chargeFlashT;
        float cx = CX + shakeX, cy = CY + shakeY;
        float r = TOWER_RADIUS * pulse;

        blendMode(BlendMode::additive);

        // charge flash: extra white glow
        if (charge > 0.f) {
            noStroke();
            for (int i = 4; i >= 0; --i) {
                float t = (float)i / 4.f;
                fill(200, 255, 240, (int)(charge * 120.f * (1.f - t)));
                polygon(cx, cy, r + t * 35.f, 6, PI / 6.f);
            }
        }

        glowPolygon(cx, cy, r, 6, PI / 6.f, 30, 140, 255, 5, 22.f);

        noStroke();
        fill(140, 210, 255, 220);
        polygon(cx, cy, r, 6, PI / 6.f);

        fill(210, 235, 255, 180);
        polygon(cx, cy, r * 0.5f, 6, PI / 6.f);

        // inner rotating hex
        noFill();
        stroke(100, 200, 255, (int)(80.f + 40.f * std::sin(towerPulseT * 4.f)));
        strokeWeight(1.f);
        polygon(cx, cy, r * 0.72f, 6, PI / 6.f + towerPulseT * 0.8f);

        noFill();
        stroke(180, 230, 255, 200);
        strokeWeight(1.5f);
        polygon(cx, cy, r, 6, PI / 6.f);

        // pulsing rings
        float ring = 0.5f + 0.5f * std::sin(bgTime * 1.8f);
        for (int i = 0; i < 3; ++i) {
            float rr = r * (2.2f + i * 1.4f) + ring * 5.f;
            stroke(50, 160, 255, (int)(22.f * (1.f - (float)i / 3.f)));
            strokeWeight(1.f);
            circle(cx, cy, rr * 2.f);
        }

        // HP ring (arc around tower)
        blendMode(BlendMode::alpha);
        drawHPRing(cx, cy, r);
    }

    void drawHPRing(float cx, float cy, float r)
    {
        float ratio = towerHp / towerMaxHp();
        color_t hpCol = lerp(rgba(255, 50, 50), rgba(60, 230, 120), ratio);
        float sweep = ratio * TAU;

        noFill();
        // background track
        stroke(30, 30, 60, 160);
        strokeWeight(3.5f);
        arc(cx, cy, (r + 14.f) * 2.f, (r + 14.f) * 2.f, -PI * 0.5f, TAU, ArcMode::open);

        // HP arc
        blendMode(BlendMode::additive);
        stroke(red(hpCol), green(hpCol), blue(hpCol), 200);
        strokeWeight(3.5f);
        arc(cx, cy, (r + 14.f) * 2.f, (r + 14.f) * 2.f, -PI * 0.5f, sweep, ArcMode::open);
        // glow on HP arc
        stroke(red(hpCol), green(hpCol), blue(hpCol), 60);
        strokeWeight(7.f);
        arc(cx, cy, (r + 14.f) * 2.f, (r + 14.f) * 2.f, -PI * 0.5f, sweep, ArcMode::open);
        blendMode(BlendMode::alpha);
    }

    // ── Enemy trails + bodies ─────────────────────────────────────────────────
    void drawEnemyTrails()
    {
        blendMode(BlendMode::additive);
        noStroke();
        for (const auto& e : enemies) {
            for (int i = 0; i < e.trailLen; ++i) {
                float t = (float)i / e.trailLen;
                float hs = e.size * 0.35f * t;
                fill(255, 60, 40, (int)(35.f * t));
                pushMatrix();
                translate(e.trail[i].x, e.trail[i].y);
                rotate(e.angle * (1.f - t));
                rect(-hs, -hs, hs * 2.f, hs * 2.f);
                popMatrix();
            }
        }
    }

    void drawEnemies()
    {
        for (const auto& e : enemies) {
            float hpRatio = e.hp / e.maxHp;
            // colour shifts orange->red as HP drops
            color_t col = rgba(255, (int)(70.f * hpRatio), 40);
            // proximity danger: glow intensifies when close to tower
            float proxRatio = 1.f - std::min(1.f, length(e.pos - float2 {CX, CY}) / shootRadius());
            if (e.flashTimer > 0.f) col = lerp(col, rgba(255, 255, 255), e.flashTimer);

            pushMatrix();
            translate(e.pos.x, e.pos.y);
            rotate(e.angle);

            blendMode(BlendMode::additive);
            noStroke();
            // glow layers (extra layer when close to tower)
            int glowLayers = (proxRatio > 0.5f) ? 5 : 3;
            float glowExtra = proxRatio * 12.f;
            for (int i = glowLayers; i >= 0; --i) {
                float t = (float)i / glowLayers;
                float ext = t * (11.f + glowExtra);
                float hs = e.size * 0.5f + ext;
                fill(red(col), green(col), blue(col), (int)((70.f + proxRatio * 40.f) * (1.f - t)));
                rect(-hs, -hs, hs * 2.f, hs * 2.f);
            }
            blendMode(BlendMode::alpha);
            fill(red(col), green(col), blue(col), 200);
            float hs = e.size * 0.5f;
            rect(-hs, -hs, hs * 2.f, hs * 2.f);

            blendMode(BlendMode::additive);
            noFill();
            stroke(255, 180, 160, 140);
            strokeWeight(1.f);
            rect(-hs, -hs, hs * 2.f, hs * 2.f);
            popMatrix();
        }
    }

    // ── Bullets ───────────────────────────────────────────────────────────────
    void drawBullets()
    {
        noStroke();
        for (const auto& b : bullets) {
            // per-type colour scheme
            color_t outerCol = (b.type == BulletType::Multi) ? rgba(60, 220, 255) : (b.type == BulletType::Speed) ? rgba(255, 160, 40)
                                                                                                                  : rgba(255, 230, 80);
            color_t coreCol = (b.type == BulletType::Multi) ? rgba(200, 245, 255) : (b.type == BulletType::Speed) ? rgba(255, 220, 150)
                                                                                                                  : rgba(255, 255, 210);
            float sizeBonus = (b.type == BulletType::Speed) ? 1.5f : 1.f;

            for (int i = 0; i < b.trailLen; ++i) {
                float t = (float)i / b.trailLen;
                fill(red(outerCol), green(outerCol), blue(outerCol), (int)(70.f * t * t));
                circle(b.trail[i].x, b.trail[i].y, BULLET_R * t * 2.f * sizeBonus);
            }
            for (int i = 3; i >= 0; --i) {
                float t = (float)i / 3.f;
                fill(red(outerCol), green(outerCol), blue(outerCol), (int)(180.f * (1.f - t)));
                circle(b.pos.x, b.pos.y, (BULLET_R + t * 8.f) * 2.f * sizeBonus);
            }
            fill(red(coreCol), green(coreCol), blue(coreCol), 240);
            circle(b.pos.x, b.pos.y, BULLET_R * 2.f * sizeBonus);
        }
    }

    // ── Particles ─────────────────────────────────────────────────────────────
    void drawParticles()
    {
        for (const auto& p : particles) {
            float t = (float)(p.life / p.maxLife);
            int a = (int)(alpha(p.col) * t * t);
            fill(red(p.col), green(p.col), blue(p.col), a);
            if (p.isSquare) {
                pushMatrix();
                translate(p.pos.x, p.pos.y);
                rotate(p.angle);
                float hs = p.size * t;
                rect(-hs, -hs, hs * 2.f, hs * 2.f);
                popMatrix();
            } else {
                circle(p.pos.x, p.pos.y, p.size * t * 2.f);
            }
        }
    }

    // ── Floating texts ────────────────────────────────────────────────────────
    void drawFloatingTexts()
    {
        blendMode(BlendMode::additive);
        for (const auto& ft : floatingTexts) {
            float t = ft.life / ft.maxLife;
            float sc = 1.f + (ft.startScale - 1.f) * std::pow(t, 0.4f); // pop scale
            int a = (int)(210.f * std::min(1.f, t * 4.f) * t);          // quick fade in, slow fade out
            fill(red(ft.col), green(ft.col), blue(ft.col), a);
            textSize(13.f * sc);
            textAlign(HorizontalTextAlign::center, VerticalTextAlign::center);
            text(ft.text, ft.pos.x, ft.pos.y);
        }
        blendMode(BlendMode::alpha);
    }

    // ── Damage flash ──────────────────────────────────────────────────────────
    void drawDamageFlash()
    {
        if (damageFlashT <= 0.f) return;
        float t = damageFlashT;
        // red edge vignette
        blendMode(BlendMode::additive);
        for (int i = 0; i < 3; ++i) {
            float fi = (float)i / 3.f;
            int a = (int)(t * 80.f * (1.f - fi));
            fill(255, 20, 20, a);
            noStroke();
            // four edge rects
            rect(0, 0, (float)W, 40.f * (1.f + fi));                     // top
            rect(0, H - 40.f * (1.f + fi), (float)W, 40.f * (1.f + fi)); // bottom
            rect(0, 0, 40.f * (1.f + fi), (float)H);                     // left
            rect(W - 40.f * (1.f + fi), 0, 40.f * (1.f + fi), (float)H); // right
        }
        blendMode(BlendMode::alpha);
    }

    // ── HUD ───────────────────────────────────────────────────────────────────
    void drawHUD()
    {
        // HP bar (slim, at bottom)
        const float bw = 220.f, bh = 10.f, bx = CX - bw * 0.5f, by = H - 32.f;
        noStroke();
        fill(15, 15, 35, 210);
        rect(bx - 2.f, by - 2.f, bw + 4.f, bh + 4.f);
        float ratio = towerHp / towerMaxHp();
        color_t hpCol = lerp(rgba(255, 50, 50), rgba(60, 230, 120), ratio);
        fill(hpCol);
        rect(bx, by, bw * ratio, bh);
        blendMode(BlendMode::additive);
        fill(red(hpCol), green(hpCol), blue(hpCol), 50);
        rect(bx, by, bw * ratio, bh);
        blendMode(BlendMode::alpha);

        fill(170, 210, 255, 185);
        textSize(11.f);
        textAlign(HorizontalTextAlign::center, VerticalTextAlign::center);
        text("HP  " + std::to_string((int)towerHp) + " / " + std::to_string((int)towerMaxHp()), CX, by + bh * 0.5f);

        fill(160, 210, 255, 210);
        textSize(15.f);
        textAlign(HorizontalTextAlign::left, VerticalTextAlign::top);
        text("Wave  " + std::to_string(wave), 14.f, 14.f);
        textAlign(HorizontalTextAlign::right, VerticalTextAlign::top);
        text("Score  " + std::to_string(score), (float)W - 14.f, 14.f);

        fill(255, 210, 50, 220);
        textSize(14.f);
        textAlign(HorizontalTextAlign::left, VerticalTextAlign::bottom);
        text("Coins:  " + std::to_string(coins) + " c", 14.f, (float)H - 14.f);

        fill(120, 160, 200, 140);
        textSize(10.f);
        textAlign(HorizontalTextAlign::right, VerticalTextAlign::bottom);
        text("U = upgrades", (float)W - 14.f, (float)H - 14.f);

        if (speedIndicatorT > 0.f) {
            blendMode(BlendMode::additive);
            fill(80, 255, 190, (int)(220.f * speedIndicatorT));
            textSize(13.f);
            textAlign(HorizontalTextAlign::center, VerticalTextAlign::top);
            text("SPEED SHOT!", CX, 14.f);
            blendMode(BlendMode::alpha);
        }
    }

    // ── Wave banner ───────────────────────────────────────────────────────────
    void drawWaveBanner()
    {
        if (waveBannerT <= 0.f) return;
        // total 2.2s: 0.3 fade-in, 1.6 hold, 0.3 fade-out
        const float dur = 2.2f;
        float t = waveBannerT / dur;
        float alphaT = (t > 0.86f) ? (1.f - t) / 0.14f : // fade in
                           (t < 0.14f) ? t / 0.14f
                                       : 1.f; // fade out
        float scaleT = 1.f + 0.12f * (1.f - std::min(1.f, (1.f - t) * 5.f));

        blendMode(BlendMode::additive);
        // glow rings around text
        noStroke();
        for (int i = 3; i >= 0; --i) {
            float fi = (float)i / 3.f;
            fill(255, 200, 50, (int)(alphaT * 25.f * (1.f - fi)));
            circle(CX, CY, (100.f + fi * 80.f) * scaleT * 2.f);
        }
        fill(255, 210, 60, (int)(alphaT * 220.f));
        textSize(44.f * scaleT);
        textAlign(HorizontalTextAlign::center, VerticalTextAlign::center);
        text(waveBannerText, CX, CY);
        blendMode(BlendMode::alpha);
    }

    // ── Shop overlay ─────────────────────────────────────────────────────────
    void drawShop()
    {
        blendMode(BlendMode::alpha);
        fill(4, 6, 22, 210);
        noStroke();
        rect(0, 0, (float)W, (float)H);

        const float pw = 730.f, ph = 420.f;
        const float px = (W - pw) * 0.5f, py = (H - ph) * 0.5f;
        fill(10, 12, 35, 240);
        rect(px, py, pw, ph, 8.f, 8.f);
        noFill();
        blendMode(BlendMode::additive);
        stroke(60, 120, 255, 120);
        // strokeWeight(1.5f);
        strokeWeight(5.5f);
        rect(px, py, pw, ph, 8.f, 8.f);
        blendMode(BlendMode::alpha);

        fill(180, 220, 255, 230);
        textSize(22.f);
        textAlign(HorizontalTextAlign::center, VerticalTextAlign::top);
        text("UPGRADES", CX, py + 14.f);

        fill(255, 210, 50, 230);
        textSize(14.f);
        text("Coins:  " + std::to_string(coins) + " c", CX, py + 44.f);

        const int cols = 4;
        const float cardW = 160.f, cardH = 140.f;
        const float gapX = 14.f, gapY = 14.f;
        const float gridW = cols * cardW + (cols - 1) * gapX;
        const float gridX = (W - gridW) * 0.5f;
        const float gridY = py + 76.f;

        for (int id = 0; id < UP_COUNT; ++id) {
            int row = id / cols, col = id % cols;
            float cx = gridX + col * (cardW + gapX);
            float cy = gridY + row * (cardH + gapY);
            bool sel = id == shopSel, buy = upgrades[id].canBuy(coins), mx = upgrades[id].maxed();

            if (sel) {
                blendMode(BlendMode::additive);
                fill(buy ? 20 : 60, buy ? 80 : 20, buy ? 180 : 120, 80);
                noStroke();
                rect(cx, cy, cardW, cardH, 6.f, 6.f);
                blendMode(BlendMode::alpha);
            }
            noFill();
            blendMode(BlendMode::additive);
            color_t bc = mx ? rgba(80, 255, 80, sel ? 200 : 80) : buy ? rgba(60, 200, 255, sel ? 220 : 90)
                                                                      : rgba(80, 80, 120, sel ? 160 : 60);
            stroke(red(bc), green(bc), blue(bc), alpha(bc));
            strokeWeight(sel ? 2.f : 1.f);
            rect(cx, cy, cardW, cardH, 6.f, 6.f);
            blendMode(BlendMode::alpha);

            fill(mx ? 80 : 200, mx ? 255 : 220, mx ? 80 : 255, 220);
            textSize(13.f);
            textAlign(HorizontalTextAlign::center, VerticalTextAlign::top);
            text(upgrades[id].name, cx + cardW * 0.5f, cy + 10.f);

            // level bar
            {
                int lv = upgrades[id].level, lmax = upgrades[id].maxLevel;
                float bw = (cardW - 20.f), sqW = (bw - (lmax - 1) * 3.f) / lmax;
                float bby = cy + 30.f;
                for (int i = 0; i < lmax; ++i) {
                    float sx = cx + 10.f + i * (sqW + 3.f);
                    noStroke();
                    if (i < lv) {
                        blendMode(BlendMode::additive);
                        fill(40, 160, 255, 180);
                    } else {
                        blendMode(BlendMode::alpha);
                        fill(30, 30, 60, 180);
                    }
                    rect(sx, bby, sqW, 6.f, 1.f, 1.f);
                }
                blendMode(BlendMode::alpha);
            }

            fill(140, 180, 220, 180);
            textSize(10.f);
            textAlign(HorizontalTextAlign::center, VerticalTextAlign::top);
            text("Lv " + std::to_string(upgrades[id].level) + " / " + std::to_string(upgrades[id].maxLevel), cx + cardW * 0.5f, cy + 42.f);

            fill(180, 210, 255, 190);
            textSize(11.f);
            text(upgrades[id].effectLine, cx + cardW * 0.5f, cy + 60.f);

            std::string cur;
            switch (id) {
                case UP_RANGE: cur = std::to_string((int)shootRadius()) + " px"; break;
                case UP_FIRERATE: cur = std::format("{:.2f} s", shootCooldown()); break;
                case UP_MULTISHOT: cur = std::format("{:.0f} %", multiShotChance() * 100); break;
                case UP_SPEEDSHOT: cur = std::format("{:.0f} %", speedShotChance() * 100); break;
                case UP_BURSTSIZE: cur = std::to_string(burstMaxShots()) + " shots"; break;
                case UP_DAMAGE: cur = std::format("{:.1f} dmg", bulletDamage()); break;
                case UP_MAXHP: cur = std::to_string((int)towerMaxHp()) + " HP"; break;
                case UP_COINBONUS: cur = std::format("{:.1f}x", coinMultiplier()); break;
            }
            fill(255, 200, 100, 180);
            textSize(10.f);
            text(cur, cx + cardW * 0.5f, cy + 78.f);

            if (mx) {
                fill(80, 255, 80, 200);
                textSize(12.f);
                text("MAXED", cx + cardW * 0.5f, cy + 108.f);
            } else {
                color_t cc = buy ? rgba(255, 210, 50) : rgba(160, 120, 60);
                fill(red(cc), green(cc), blue(cc), buy ? 220 : 150);
                textSize(12.f);
                text(std::to_string(upgrades[id].nextCost()) + " c", cx + cardW * 0.5f, cy + 108.f);
            }
        }

        fill(120, 160, 200, 160);
        textSize(11.f);
        textAlign(HorizontalTextAlign::center, VerticalTextAlign::bottom);
        text("Arrow Keys  Navigate     Enter  Buy     U / ESC  Close", CX, py + ph - 12.f);
    }

    // ── Menu ─────────────────────────────────────────────────────────────────
    void drawMenu()
    {
        blendMode(BlendMode::additive);
        float pulse = 1.f + 0.06f * std::sin(bgTime * 2.f);
        glowPolygon(CX, CY - 100.f, TOWER_RADIUS * pulse * 2.f, 6, PI / 6.f, 30, 140, 255, 5, 30.f);
        noStroke();
        fill(140, 215, 255, 200);
        polygon(CX, CY - 100.f, TOWER_RADIUS * pulse * 2.f, 6, PI / 6.f);
        for (int i = 0; i < 4; ++i) {
            float t = i / 4.f;
            fill(20, 80, 255, (int)(35.f * (1.f - t)));
            circle(CX, CY - 100.f, (110.f + t * 70.f + 15.f * std::sin(bgTime * 1.1f + t)) * 2.f);
        }
        blendMode(BlendMode::alpha);
        fill(200, 225, 255, 230);
        textSize(52.f);
        textAlign(HorizontalTextAlign::center, VerticalTextAlign::center);
        text("THE TOWER", CX, CY + 5.f);
        fill(120, 175, 220, 180);
        textSize(15.f);
        text("Defend against endless waves of enemies", CX, CY + 50.f);
        text("Press  ENTER  to begin", CX, CY + 78.f);
        fill(140, 185, 220, 160);
        textSize(11.f);
        text("During play:  U = Upgrade Shop   ESC = Menu", CX, CY + 108.f);
        if (highScore > 0) {
            fill(255, 210, 60, 200);
            textSize(13.f);
            text("High Score:  " + std::to_string(highScore), CX, CY + 138.f);
        }
    }

    // ── Game Over ─────────────────────────────────────────────────────────────
    void drawGameOver()
    {
        blendMode(BlendMode::alpha);
        fill(4, 4, 14, 185);
        noStroke();
        rect(0, 0, (float)W, (float)H);
        blendMode(BlendMode::additive);
        fill(255, 50, 50, 210);
        textSize(52.f);
        textAlign(HorizontalTextAlign::center, VerticalTextAlign::center);
        text("GAME OVER", CX, CY - 65.f);
        blendMode(BlendMode::alpha);
        fill(200, 200, 255, 200);
        textSize(20.f);
        text("Wave  " + std::to_string(wave), CX, CY);
        text("Score:  " + std::to_string(score), CX, CY + 34.f);
        if (score > 0 && score == highScore) {
            fill(255, 220, 60, 220);
            textSize(15.f);
            text("New High Score!", CX, CY + 68.f);
        }
        fill(140, 195, 255, 180);
        textSize(13.f);
        text("Press  ENTER  to play again", CX, CY + 110.f);
    }
};

std::unique_ptr<p5::Sketch> p5::createSketch()
{
    return std::make_unique<TowerSketch>();
}
