#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>
#include <p5cpp_audio/p5cpp_audio.hpp>

using namespace p5;
using namespace p5::audio;
using namespace p5::animation;

namespace
{
    constexpr float TILE_SIZE = 32.0f;
    constexpr int WORLD_WIDTH_TILES = 60;
    constexpr int WORLD_HEIGHT_TILES = 40;

    // Caps a single frame's delta time. Without this, one abnormally long frame (alt-tab, a
    // breakpoint, the window being dragged) turns into an enormous position/velocity step that
    // frame -- which can propagate into non-finite (NaN/Inf) values from there (e.g. two entities'
    // positions ending up so far apart that subtracting them overflows), corrupting everything
    // downstream including render vertices ("tesselate_polygon: non-finite vertex position...").
    constexpr double MAX_DELTA_TIME = 0.1; // treat any longer frame as if it were 10 FPS

    constexpr float PLAYER_SPEED = 220.0f; // pixels per second
    constexpr float PLAYER_RADIUS = 10.0f;
    constexpr float CAMERA_FOLLOW_FACTOR = 0.1f;

    constexpr float STARTING_GOLD = 50.0f;

    constexpr float PLAYER_MAX_HEALTH = 100.0f;
    constexpr float PLAYER_FIRE_RANGE = 220.0f;  // auto-fire only engages enemies within this radius
    constexpr float PLAYER_FIRE_INTERVAL = 0.4f; // seconds between shots

    constexpr float ENEMY_SPEED = 80.0f;
    constexpr float ENEMY_RADIUS = 9.0f;
    constexpr float ENEMY_MAX_HEALTH = 3.0f;
    constexpr float ENEMY_CONTACT_DAMAGE = 15.0f; // per hit, not per second -- see playerInvulnerabilityTimer
    constexpr float ENEMY_KILL_REWARD_GOLD = 5.0f;
    constexpr float ENEMY_SPAWN_RADIUS = 650.0f; // spawn just outside view and walk in
    constexpr float ENEMY_HIT_FLASH_DURATION = 0.12f;
    constexpr float ENEMY_DEATH_DURATION = 0.25f; // shrink/fade-out before an enemy is actually removed

    constexpr float PLAYER_INVULNERABILITY_DURATION = 0.6f; // i-frames after taking a contact hit

    constexpr double WAVE_INTERVAL_SECONDS = 25.0;
    constexpr int WAVE_BASE_ENEMY_COUNT = 3;
    constexpr int WAVE_ENEMY_INCREMENT = 2; // each wave brings this many more enemies than the last

    constexpr float PROJECTILE_SPEED = 420.0f;
    constexpr float PROJECTILE_RADIUS = 4.0f;
    constexpr float PROJECTILE_DAMAGE = 1.0f;                // base damage; weaponLevel upgrades add on top
    constexpr float PROJECTILE_MAX_TRAVEL_DISTANCE = 700.0f; // despawn a shot that hit nothing

    // Wood/food were pure idle counters until now -- these upgrades are their first real sink,
    // and directly counter the escalating waves (more damage, more max HP).
    constexpr float WEAPON_UPGRADE_BASE_COST = 15.0f; // in wood; scales as cost * (level + 1)
    constexpr float WEAPON_UPGRADE_DAMAGE_BONUS = 1.0f;
    constexpr float VITALITY_UPGRADE_BASE_COST = 15.0f; // in food; scales as cost * (level + 1)
    constexpr float VITALITY_UPGRADE_HEALTH_BONUS = 20.0f;

    constexpr float MUZZLE_FLASH_DURATION = 0.08f;

    constexpr float CAMERA_HIT_SHAKE_MAGNITUDE = 5.0f;
    constexpr float CAMERA_HIT_SHAKE_DURATION = 0.15f;

    constexpr float BUILDING_POP_DURATION = 0.35f; // scale-in animation when a building is placed

    // A dash gives the player something active to do besides walk into auto-fire range -- a
    // short speed burst with i-frames (see updateEnemies), not just a sprint.
    constexpr float PLAYER_DASH_SPEED_MULTIPLIER = 3.0f;
    constexpr float PLAYER_DASH_DURATION = 0.15f;
    constexpr float PLAYER_DASH_COOLDOWN = 0.7f;

    constexpr float PLAYER_BOB_FREQUENCY = 9.0f; // radians/sec while moving, tuned by feel
    constexpr float PLAYER_BOB_AMPLITUDE = 2.5f;

    // Sparse, fixed decorations scattered over grass at world-generation time -- cheap texture
    // variation without turning the (mostly solid-color) background into a per-tile draw call.
    constexpr float GRASS_DECORATION_CHANCE = 0.35f;
    constexpr float GRASS_DECORATION_RADIUS = 2.0f;

    const color_t GRASS_COLOR = rgba(58, 102, 53);
    const color_t GRASS_DECORATION_COLOR = rgba(46, 87, 42);
    const color_t PATH_COLOR = rgba(178, 151, 104);
    const color_t PLAYER_COLOR = rgba(220, 60, 60);
    const color_t ENEMY_COLOR = rgba(150, 40, 165);
    const color_t PROJECTILE_HEAD_COLOR = rgba(235, 225, 210); // arrowhead: flint/steel grey-white
    const color_t PROJECTILE_SHAFT_COLOR = rgba(120, 82, 44);  // shaft + fletching: wood brown
    const color_t MUZZLE_FLASH_COLOR = rgba(255, 245, 180);
    const color_t PLAYER_DASH_COLOR = rgba(255, 215, 215);

    // Parchment/wood HUD theme -- coordinated beige panels with brown text/borders instead of the
    // previous plain black boxes with white text.
    const color_t HUD_PANEL_COLOR = rgba(235, 220, 185, 235);
    const color_t HUD_PANEL_BORDER_COLOR = rgba(120, 85, 45);
    const color_t HUD_TEXT_COLOR = rgba(74, 48, 24);
    const color_t HUD_TEXT_SUBTLE_COLOR = rgba(120, 95, 65); // secondary text on parchment panels
    const color_t HUD_TEXT_MUTED_COLOR = rgba(255, 235, 210);
    const color_t HUD_SELECTED_COLOR = rgba(214, 168, 62);
    const color_t GAME_OVER_TEXT_COLOR = rgba(245, 230, 200);
} // namespace

namespace
{
    // Deterministic per-tile hash for subtle color variation -- the same tile always gets the
    // same jitter, so terrain doesn't flicker/shimmer as the camera pans past it.
    constexpr uint32_t hashTile(int x, int y)
    {
        uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(y) * 668265263u;
        h = (h ^ (h >> 13)) * 1274126177u;
        return h ^ (h >> 16);
    }

    // Used to derive a building icon's roof/shadow/outline shades from its one "identity" color
    // (BuildingInfo::color) instead of hardcoding unrelated colors per icon.
    color_t darken(color_t color, float factor)
    {
        return rgba(static_cast<int>(getRed(color) * factor), static_cast<int>(getGreen(color) * factor), static_cast<int>(getBlue(color) * factor));
    }
} // namespace

namespace
{
    // Minimal 16-bit mono PCM WAV encoder plus a handful of procedurally synthesized sound
    // effects -- this keeps the example free of audio asset files, matching the "no asset
    // loading" choice already made for the visuals.
    std::vector<uint8_t> encodeWav(const std::vector<int16_t>& samples, uint32_t sampleRate)
    {
        const auto dataSize = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
        const uint32_t byteRate = sampleRate * sizeof(int16_t);

        std::vector<uint8_t> wav;
        wav.reserve(44 + dataSize);

        const auto writeU32 = [&wav](uint32_t value) {
            for (int i = 0; i < 4; ++i) wav.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
        };
        const auto writeU16 = [&wav](uint16_t value) {
            for (int i = 0; i < 2; ++i) wav.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
        };
        const auto writeTag = [&wav](const char* tag) {
            wav.insert(wav.end(), tag, tag + 4);
        };

        writeTag("RIFF");
        writeU32(36 + dataSize);
        writeTag("WAVE");
        writeTag("fmt ");
        writeU32(16);
        writeU16(1); // PCM
        writeU16(1); // mono
        writeU32(sampleRate);
        writeU32(byteRate);
        writeU16(2);  // block align
        writeU16(16); // bits per sample
        writeTag("data");
        writeU32(dataSize);

        for (const int16_t sample : samples) {
            writeU16(static_cast<uint16_t>(sample));
        }

        return wav;
    }

    // Peak is deliberately a bit under full scale (32767) -- pinning every effect right at the
    // ceiling is what made the original pass sound hot/clippy rather than punchy.
    std::vector<int16_t> quantizeSamples(const std::vector<float>& floatSamples)
    {
        std::vector<int16_t> samples(floatSamples.size());
        for (size_t i = 0; i < floatSamples.size(); ++i) {
            samples[i] = static_cast<int16_t>(constrain(floatSamples[i], -1.0f, 1.0f) * 26000.0f);
        }
        return samples;
    }

    std::vector<uint8_t> synthesizeWav(float durationSeconds, uint32_t sampleRate, const std::function<float(float)>& amplitudeAt)
    {
        const auto sampleCount = static_cast<uint32_t>(durationSeconds * static_cast<float>(sampleRate));
        std::vector<float> floatSamples(sampleCount);
        for (uint32_t i = 0; i < sampleCount; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(sampleRate);
            floatSamples[i] = amplitudeAt(t);
        }
        return encodeWav(quantizeSamples(floatSamples), sampleRate);
    }

    // A hard jump straight to full amplitude at sample 0 is an audible "click" -- every generator
    // below multiplies its envelope by this short linear fade-in to avoid that discontinuity.
    float attackEnvelope(float t, float attackSeconds)
    {
        return attackSeconds <= 0.0f ? 1.0f : constrain(t / attackSeconds, 0.0f, 1.0f);
    }

    std::vector<uint8_t> synthesizeShootWav()
    {
        return synthesizeWav(0.09f, 44100, [](float t) {
            const float frequency = lerp(1100.0f, 260.0f, t / 0.09f);
            const float envelope = attackEnvelope(t, 0.004f) * std::exp(-t * 22.0f);
            // A touch of 2nd-harmonic on top of the fundamental gives the laser some body instead
            // of a thin pure sine.
            return envelope * (0.8f * std::sin(TAU * frequency * t) + 0.2f * std::sin(TAU * frequency * 2.0f * t));
        });
    }

    std::vector<uint8_t> synthesizeHitWav()
    {
        // Raw white noise alone reads as harsh static, not an impact -- this layers a fast
        // downward sine "thump" underneath a low-pass-filtered noise texture instead.
        constexpr uint32_t sampleRate = 44100;
        constexpr float duration = 0.12f;
        const auto sampleCount = static_cast<uint32_t>(duration * static_cast<float>(sampleRate));

        std::vector<float> samples(sampleCount);
        float filteredNoise = 0.0f;
        for (uint32_t i = 0; i < sampleCount; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(sampleRate);
            const float envelope = attackEnvelope(t, 0.002f) * std::exp(-t * 32.0f);

            const float noise = random(2.0f) - 1.0f;
            filteredNoise += (noise - filteredNoise) * 0.35f; // one-pole low-pass: softens static into a "crunch"

            const float thumpFrequency = lerp(220.0f, 70.0f, t / duration);
            const float thump = std::sin(TAU * thumpFrequency * t);

            samples[i] = envelope * (0.55f * thump + 0.45f * filteredNoise);
        }

        return encodeWav(quantizeSamples(samples), sampleRate);
    }

    std::vector<uint8_t> synthesizePlaceWav()
    {
        return synthesizeWav(0.1f, 44100, [](float t) {
            const float frequency = lerp(440.0f, 720.0f, t / 0.1f);
            const float envelope = attackEnvelope(t, 0.006f) * std::exp(-t * 16.0f);
            return envelope * std::sin(TAU * frequency * t);
        });
    }

    std::vector<uint8_t> synthesizeWaveStartWav()
    {
        return synthesizeWav(0.4f, 44100, [](float t) {
            if (t < 0.16f) {
                return attackEnvelope(t, 0.006f) * std::exp(-t * 10.0f) * std::sin(TAU * 640.0f * t);
            }
            if (t < 0.22f) {
                return 0.0f; // brief gap between the two beeps
            }
            const float localT = t - 0.22f;
            return attackEnvelope(localT, 0.006f) * std::exp(-localT * 10.0f) * std::sin(TAU * 880.0f * localT);
        });
    }

    std::vector<uint8_t> synthesizeGameOverWav()
    {
        return synthesizeWav(0.8f, 44100, [](float t) {
            const float frequency = lerp(440.0f, 80.0f, t / 0.8f);
            const float envelope = attackEnvelope(t, 0.01f) * std::exp(-t * 2.6f);
            // A quiet sub-harmonic underneath the main tone gives the descent more weight.
            return envelope * (0.7f * std::sin(TAU * frequency * t) + 0.3f * std::sin(TAU * frequency * 0.5f * t));
        });
    }

    std::vector<uint8_t> synthesizeUpgradeWav()
    {
        return synthesizeWav(0.28f, 44100, [](float t) {
            if (t < 0.12f) {
                return attackEnvelope(t, 0.006f) * std::exp(-t * 13.0f) * std::sin(TAU * 520.0f * t);
            }
            const float localT = t - 0.12f;
            return attackEnvelope(localT, 0.006f) * std::exp(-localT * 11.0f) * std::sin(TAU * 780.0f * localT);
        });
    }
} // namespace

enum class TileType
{
    grass,
    path,
};

// Every building costs gold to place (the only spendable resource so far) but produces exactly
// one of three resources -- this is what turns "place a building" into an actual economic choice
// instead of a single idle counter. Wood/food aren't spent on anything yet; they're tracked so a
// later step (e.g. wall/troop costs) has real numbers to hook into.
enum class BuildingType
{
    none,
    goldMine,
    sawmill,
    farm,
    Count,
};

struct BuildingInfo
{
    std::string_view name;
    color_t color;
    float cost;
    float goldPerSecond;
    float woodPerSecond;
    float foodPerSecond;
};

inline const BuildingInfo& getBuildingInfo(BuildingType type)
{
    static const BuildingInfo goldMine {"Gold Mine", rgba(218, 165, 32), 20.0f, 1.0f, 0.0f, 0.0f}; // goldenrod
    static const BuildingInfo sawmill {"Sawmill", rgba(101, 67, 33), 15.0f, 0.0f, 1.0f, 0.0f};     // wood brown
    static const BuildingInfo farm {"Farm", rgba(150, 195, 74), 25.0f, 0.0f, 0.0f, 1.0f};          // crop green
    static const BuildingInfo none {"", rgba(0, 0), 0.0f, 0.0f, 0.0f, 0.0f};

    switch (type) {
        case BuildingType::goldMine: return goldMine;
        case BuildingType::sawmill: return sawmill;
        case BuildingType::farm: return farm;
        default: return none;
    }
}

struct World
{
    int width = WORLD_WIDTH_TILES;
    int height = WORLD_HEIGHT_TILES;
    std::vector<TileType> tiles;
    std::vector<BuildingType> buildings;
    std::array<int, static_cast<size_t>(BuildingType::Count)> buildingCounts {};
    std::vector<float2> grassDecorations;          // fixed world-space accents, regenerated with the world
    std::vector<tween<float>> buildingScaleTweens; // per-tile "pop in" animation, restarted on placement

    void generate()
    {
        tiles.assign(static_cast<size_t>(width) * height, TileType::grass);
        buildings.assign(static_cast<size_t>(width) * height, BuildingType::none);
        buildingCounts.fill(0);
        buildingScaleTweens.assign(static_cast<size_t>(width) * height, createTween(0.0f, 1.0f, BUILDING_POP_DURATION, easeOutBack));

        // Simple deterministic street grid: every 8th column and every 6th row is a path,
        // carving a village-like block layout out of the grass. No noise/assets yet -- this
        // is just the placeholder terrain for the movement/camera foundation.
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (x % 8 == 0 || y % 6 == 0) {
                    at(x, y) = TileType::path;
                }
            }
        }

        generateGrassDecorations();
    }

    // Scatters small accent points over grass tiles once, at generation time, so the background
    // reads as textured terrain instead of one flat color -- without paying for a per-tile draw
    // call every frame the way the (much sparser) path tiles do.
    void generateGrassDecorations()
    {
        grassDecorations.clear();
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                // Qualified: an unqualified random() is ambiguous with the zero-arg POSIX
                // ::random() that <cstdlib> pulls into the global namespace on this platform.
                if (at(x, y) != TileType::grass || p5::random() > GRASS_DECORATION_CHANCE) {
                    continue;
                }
                const float2 tileOrigin = float2 {x * TILE_SIZE, y * TILE_SIZE};
                grassDecorations.push_back(tileOrigin + float2 {random(TILE_SIZE), random(TILE_SIZE)});
            }
        }
    }

    TileType& at(int x, int y) { return tiles[static_cast<size_t>(y) * width + x]; }

    TileType at(int x, int y) const
    {
        if (x < 0 || y < 0 || x >= width || y >= height) {
            return TileType::grass;
        }
        return tiles[static_cast<size_t>(y) * width + x];
    }

    BuildingType buildingTypeAt(int x, int y) const
    {
        if (x < 0 || y < 0 || x >= width || y >= height) {
            return BuildingType::none;
        }
        return buildings[static_cast<size_t>(y) * width + x];
    }

    void setBuilding(int x, int y, BuildingType type)
    {
        if (x < 0 || y < 0 || x >= width || y >= height) {
            return;
        }

        const size_t index = static_cast<size_t>(y) * width + x;
        const BuildingType previous = buildings[index];
        if (previous == type) {
            return; // no-op re-placement/removal keeps buildingCounts accurate
        }

        buildings[index] = type;
        if (previous != BuildingType::none) buildingCounts[static_cast<size_t>(previous)]--;
        if (type != BuildingType::none) {
            buildingCounts[static_cast<size_t>(type)]++;
            restart(buildingScaleTweens[index]); // pop in from scale 0 whenever a *new* building appears
        }
    }

    void advanceBuildingAnimations(double deltaTime)
    {
        const float dt = static_cast<float>(deltaTime);
        for (tween<float>& scaleTween : buildingScaleTweens) {
            advance(scaleTween, dt); // no-op for tiles that aren't mid-animation
        }
    }

    float buildingScaleAt(int x, int y) const
    {
        if (x < 0 || y < 0 || x >= width || y >= height) {
            return 1.0f;
        }
        return value(buildingScaleTweens[static_cast<size_t>(y) * width + x]);
    }

    // Circle-vs-tile-grid check used for player movement collision: true if a building
    // occupies any tile overlapping the given circle's bounding box.
    bool isBuildingBlocking(const float2& position, float radius) const
    {
        const int minX = static_cast<int>(std::floor((position.x - radius) / TILE_SIZE));
        const int maxX = static_cast<int>(std::floor((position.x + radius) / TILE_SIZE));
        const int minY = static_cast<int>(std::floor((position.y - radius) / TILE_SIZE));
        const int maxY = static_cast<int>(std::floor((position.y + radius) / TILE_SIZE));

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                if (buildingTypeAt(x, y) != BuildingType::none) {
                    return true;
                }
            }
        }
        return false;
    }
};

struct Player
{
    float2 position {0.0f, 0.0f};
    float2 facing {0.0f, -1.0f}; // last non-zero movement direction, drives the facing indicator + dash

    bool isDashing = false;
    float dashTimer = 0.0f;
    float dashCooldownTimer = 0.0f;
    float2 dashDirection {0.0f, -1.0f};

    float animationTime = 0.0f; // only advances while actually moving -- drives the walk bob

    // Axis-separated movement so the player slides along a building's edge instead of
    // stopping dead the instant either axis would overlap it.
    void update(double deltaTime, const World& world)
    {
        const float dt = static_cast<float>(deltaTime);
        dashCooldownTimer = std::max(0.0f, dashCooldownTimer - dt);

        float2 inputDirection {0.0f, 0.0f};
        if (isKeyDown(Key::W) || isKeyDown(Key::Up)) inputDirection.y -= 1.0f;
        if (isKeyDown(Key::S) || isKeyDown(Key::Down)) inputDirection.y += 1.0f;
        if (isKeyDown(Key::A) || isKeyDown(Key::Left)) inputDirection.x -= 1.0f;
        if (isKeyDown(Key::D) || isKeyDown(Key::Right)) inputDirection.x += 1.0f;
        inputDirection = normalized(inputDirection);

        const bool isMoving = inputDirection.x != 0.0f || inputDirection.y != 0.0f;
        if (isMoving) {
            facing = inputDirection;
            animationTime += dt;
        }

        // Left Shift: a short burst of speed in the facing direction, on a cooldown -- doubles as
        // a dodge, since KingshotClone::updateEnemies skips contact damage while isDashing is true.
        if (isKeyPressed(Key::LeftShift) && !isDashing && dashCooldownTimer <= 0.0f) {
            isDashing = true;
            dashTimer = PLAYER_DASH_DURATION;
            dashCooldownTimer = PLAYER_DASH_COOLDOWN;
            dashDirection = facing;
        }

        float2 moveDirection = inputDirection;
        float speed = PLAYER_SPEED;
        if (isDashing) {
            moveDirection = dashDirection;
            speed = PLAYER_SPEED * PLAYER_DASH_SPEED_MULTIPLIER;
            dashTimer -= dt;
            if (dashTimer <= 0.0f) {
                isDashing = false;
            }
        }

        const float2 delta = moveDirection * (speed * dt);

        const float2 candidateX = position + float2 {delta.x, 0.0f};
        if (!world.isBuildingBlocking(candidateX, PLAYER_RADIUS)) {
            position.x = candidateX.x;
        }

        const float2 candidateY = position + float2 {0.0f, delta.y};
        if (!world.isBuildingBlocking(candidateY, PLAYER_RADIUS)) {
            position.y = candidateY.y;
        }
    }
};

struct Camera
{
    float2 position {0.0f, 0.0f};
    float2 shakeOffset {0.0f, 0.0f};
    float shakeMagnitude = 0.0f;
    float shakeDuration = 0.0f;
    float shakeTimer = 0.0f;

    void follow(const float2& target)
    {
        position = lerp(position, target, CAMERA_FOLLOW_FACTOR);
    }

    void shake(float magnitude, float duration)
    {
        shakeMagnitude = magnitude;
        shakeDuration = duration;
        shakeTimer = duration;
    }

    void updateShake(double deltaTime)
    {
        shakeTimer = std::max(0.0f, shakeTimer - static_cast<float>(deltaTime));
        if (shakeTimer <= 0.0f) {
            shakeOffset = float2 {0.0f, 0.0f};
            return;
        }

        const float magnitude = shakeMagnitude * (shakeTimer / shakeDuration); // decays to 0 as shakeTimer runs out
        shakeOffset = float2 {(random(2.0f) - 1.0f) * magnitude, (random(2.0f) - 1.0f) * magnitude};
    }

    // Only worldToScreen() incorporates the shake -- screenToWorld() (used to pick a tile under
    // the mouse for building placement) deliberately stays on the unshaken position, so a shake
    // in progress can never cause a misplaced click.
    float2 worldToScreen(const float2& worldPosition) const
    {
        return worldPosition - (position + shakeOffset) + float2 {getWidth() * 0.5f, getHeight() * 0.5f};
    }

    float2 screenToWorld(const float2& screenPosition) const
    {
        return screenPosition - float2 {getWidth() * 0.5f, getHeight() * 0.5f} + position;
    }
};

// loadSoundFromMemory() decodes lazily through a ma_decoder that keeps a pointer into the buffer
// it was given instead of copying it -- so a Voice's wavBytes must never be resized/reassigned
// again once loaded, and must outlive its `sound`.
//
// Overlap is handled with a small round-robin pool of independently-decoded voices rather than
// createSoundAlias()/playSoundOverlapped(): aliasing (ma_sound_init_copy) only works for sounds
// initialized through miniaudio's resource manager, not for the raw ma_decoder that
// loadSoundFromMemory() wraps -- calling it on one of these logs "Failed to create sound alias"
// every single time. Giving each voice its own decoded copy up front sidesteps that entirely.
struct SoundEffect
{
    static constexpr size_t VOICE_COUNT = 4;

    struct Voice
    {
        std::vector<uint8_t> wavBytes;
        Sound sound;
    };

    std::array<Voice, VOICE_COUNT> voices;
    size_t nextVoice = 0;

    void load(const std::vector<uint8_t>& bytes)
    {
        for (Voice& voice : voices) {
            voice.wavBytes = bytes; // each voice owns an independent copy + decoder
            voice.sound = loadSoundFromMemory(std::span<const uint8_t> {voice.wavBytes});
        }
    }

    void play()
    {
        Voice& voice = voices[nextVoice];
        nextVoice = (nextVoice + 1) % VOICE_COUNT;

        // stopSound() also seeks back to frame 0, so a voice restarts cleanly whether it was
        // still playing, already finished, or never played at all.
        stopSound(voice.sound);
        playSound(voice.sound);
    }
};

struct Enemy
{
    float2 position;
    float2 facing {0.0f, -1.0f}; // toward the player, updated every frame it's alive -- drives the icon's rotation
    float health;
    float hitFlashTimer = 0.0f;
    bool dying = false;

    // Drives the death animation's shrink-and-fade (1 -> 0); `dying` is the gameplay-facing flag
    // (skip movement/targeting/further hits), this tween is purely the visual progress behind it.
    tween<float> deathTween = createTween(1.0f, 0.0f, ENEMY_DEATH_DURATION, easeInQuad);
};

struct Projectile
{
    float2 position;
    float2 velocity;
    float traveled = 0.0f; // distance covered since firing, used to despawn a shot that hit nothing
};

enum class GameState
{
    mainMenu,
    playing,
    gameOver,
};

struct KingshotClone : public Sketch
{
    World world;
    Player player;
    Camera camera;
    float gold = STARTING_GOLD;
    float wood = 0.0f;
    float food = 0.0f;
    BuildingType selectedBuildingType = BuildingType::goldMine;

    std::vector<Enemy> enemies;
    std::vector<Projectile> projectiles;
    float maxPlayerHealth = PLAYER_MAX_HEALTH; // raised by vitality upgrades
    float playerHealth = PLAYER_MAX_HEALTH;
    float playerInvulnerabilityTimer = 0.0f;
    float projectileDamage = PROJECTILE_DAMAGE; // raised by weapon upgrades
    int weaponLevel = 0;
    int vitalityLevel = 0;
    double waveTimer = 0.0;
    int waveNumber = 0;
    double fireCooldown = 0.0;
    GameState state = GameState::mainMenu;

    float2 muzzleFlashTarget {0.0f, 0.0f};
    float muzzleFlashTimer = 0.0f;

    SoundEffect shootSound;
    SoundEffect hitSound;
    SoundEffect placeSound;
    SoundEffect waveStartSound;
    SoundEffect gameOverSound;
    SoundEffect upgradeSound;

    void setup() override
    {
        setWindowSize(960, 600);
        setWindowTitle("Kingshot Clone");

        // Loaded once here, not in resetGame() -- the synthesized bytes never change across restarts.
        shootSound.load(synthesizeShootWav());
        hitSound.load(synthesizeHitWav());
        placeSound.load(synthesizePlaceWav());
        waveStartSound.load(synthesizeWaveStartWav());
        gameOverSound.load(synthesizeGameOverWav());
        upgradeSound.load(synthesizeUpgradeWav());

        resetGame();
    }

    void resetGame()
    {
        world.generate();

        player.position = float2 {
            world.width * TILE_SIZE * 0.5f,
            world.height * TILE_SIZE * 0.5f,
        };
        camera.position = player.position;

        gold = STARTING_GOLD;
        wood = 0.0f;
        food = 0.0f;
        selectedBuildingType = BuildingType::goldMine;

        enemies.clear();
        projectiles.clear();
        maxPlayerHealth = PLAYER_MAX_HEALTH;
        playerHealth = maxPlayerHealth;
        playerInvulnerabilityTimer = 0.0f;
        projectileDamage = PROJECTILE_DAMAGE;
        weaponLevel = 0;
        vitalityLevel = 0;
        waveTimer = 0.0;
        waveNumber = 0;
        fireCooldown = 0.0;

        muzzleFlashTimer = 0.0f;
        camera.shakeTimer = 0.0f;
        camera.shakeOffset = float2 {0.0f, 0.0f};
    }

    struct TileRange
    {
        int startX, startY, endX, endY;
    };

    void draw() override
    {
        // Computed once per frame and clamped (see MAX_DELTA_TIME) instead of calling
        // getDeltaTime() repeatedly below -- a single source of truth for this frame's step, and
        // immune to one abnormally long frame blowing up position/physics math.
        const double deltaTime = std::min(getDeltaTime(), MAX_DELTA_TIME);

        if (state == GameState::mainMenu) {
            drawMainMenuScreen();
            return;
        }

        if (state == GameState::gameOver) {
            renderFrame();
            drawGameOverScreen();
            return;
        }

        handleBuildInput();
        handleUpgradeInput();
        player.update(deltaTime, world);
        camera.follow(player.position);
        camera.updateShake(deltaTime);
        playerInvulnerabilityTimer = std::max(0.0f, playerInvulnerabilityTimer - static_cast<float>(deltaTime));
        updateResources(deltaTime);
        world.advanceBuildingAnimations(deltaTime);
        updateWaves(deltaTime);
        updateEnemies(deltaTime);
        updateCombat(deltaTime);
        updateProjectiles(deltaTime);

        if (playerHealth <= 0.0f) {
            state = GameState::gameOver;
            gameOverSound.play();
        }

        renderFrame();
    }

    void renderFrame()
    {
        background(GRASS_COLOR);
        const TileRange range = computeVisibleTileRange();
        drawGrassDecorations(range);
        drawVisiblePathTiles(range);
        drawVisibleBuildings(range);
        drawEnemies();
        drawProjectiles();
        drawMuzzleFlash();
        drawPlayer();
        drawHud();
    }

    // New waves spawn just outside the visible area and walk straight toward the player -- no
    // pathfinding around buildings yet, that's a follow-up once buildings matter tactically.
    void updateWaves(double deltaTime)
    {
        waveTimer += deltaTime;
        if (waveTimer < WAVE_INTERVAL_SECONDS) {
            return;
        }
        waveTimer = 0.0;
        spawnWave();
    }

    void spawnWave()
    {
        ++waveNumber;
        waveStartSound.play();

        const int enemyCount = WAVE_BASE_ENEMY_COUNT + (waveNumber - 1) * WAVE_ENEMY_INCREMENT;
        for (int i = 0; i < enemyCount; ++i) {
            const float angle = random(TAU);
            const float2 offset = float2 {std::cos(angle), std::sin(angle)} * ENEMY_SPAWN_RADIUS;
            enemies.push_back(Enemy {.position = player.position + offset, .health = ENEMY_MAX_HEALTH});
        }
    }

    void updateEnemies(double deltaTime)
    {
        const float dt = static_cast<float>(deltaTime);
        for (Enemy& enemy : enemies) {
            enemy.hitFlashTimer = std::max(0.0f, enemy.hitFlashTimer - dt);

            if (enemy.dying) {
                advance(enemy.deathTween, dt);
                continue; // frozen in place while its death animation plays out
            }

            const float2 direction = normalized(player.position - enemy.position);
            enemy.facing = direction;
            enemy.position = enemy.position + direction * (ENEMY_SPEED * dt);

            // A single hit + a brief invulnerability window feels much fairer than a continuous
            // per-second drain while standing in contact with an enemy.
            if (playerInvulnerabilityTimer <= 0.0f && !player.isDashing && distance(enemy.position, player.position) < (ENEMY_RADIUS + PLAYER_RADIUS)) {
                playerHealth -= ENEMY_CONTACT_DAMAGE;
                playerInvulnerabilityTimer = PLAYER_INVULNERABILITY_DURATION;
                camera.shake(CAMERA_HIT_SHAKE_MAGNITUDE, CAMERA_HIT_SHAKE_DURATION);
            }
        }

        std::erase_if(enemies, [](const Enemy& enemy) {
            return enemy.dying && isFinished(enemy.deathTween);
        });
    }

    // Auto-fire: the mouse stays reserved for building, so the player just needs to be in range --
    // fires at the nearest enemy on a fixed cadence.
    void updateCombat(double deltaTime)
    {
        muzzleFlashTimer = std::max(0.0f, muzzleFlashTimer - static_cast<float>(deltaTime));

        fireCooldown -= deltaTime;
        if (fireCooldown > 0.0) {
            return;
        }

        const Enemy* target = findNearestEnemyInRange();
        if (target == nullptr) {
            return;
        }

        const float2 direction = normalized(target->position - player.position);
        projectiles.push_back(Projectile {.position = player.position, .velocity = direction * PROJECTILE_SPEED});
        fireCooldown = PLAYER_FIRE_INTERVAL;

        muzzleFlashTimer = MUZZLE_FLASH_DURATION;
        muzzleFlashTarget = target->position;
        shootSound.play();
    }

    const Enemy* findNearestEnemyInRange() const
    {
        const Enemy* nearest = nullptr;
        float nearestDistanceSquared = PLAYER_FIRE_RANGE * PLAYER_FIRE_RANGE;
        for (const Enemy& enemy : enemies) {
            if (enemy.dying) continue;

            const float distanceSquaredToEnemy = distanceSquared(player.position, enemy.position);
            if (distanceSquaredToEnemy < nearestDistanceSquared) {
                nearestDistanceSquared = distanceSquaredToEnemy;
                nearest = &enemy;
            }
        }
        return nearest;
    }

    void updateProjectiles(double deltaTime)
    {
        const float dt = static_cast<float>(deltaTime);
        for (Projectile& projectile : projectiles) {
            const float2 step = projectile.velocity * dt;
            projectile.position = projectile.position + step;
            projectile.traveled += length(step);
        }

        // Projectile-vs-enemy hit test: the first (non-dying) enemy within range absorbs the hit.
        // Dropping an enemy to 0 HP doesn't remove it immediately -- it enters its death animation
        // (see updateEnemies) so the kill actually reads as an event instead of an instant pop.
        std::erase_if(projectiles, [this](const Projectile& projectile) {
            for (Enemy& enemy : enemies) {
                if (enemy.dying || distance(projectile.position, enemy.position) >= (PROJECTILE_RADIUS + ENEMY_RADIUS)) {
                    continue;
                }

                enemy.health -= projectileDamage;
                enemy.hitFlashTimer = ENEMY_HIT_FLASH_DURATION;
                hitSound.play();

                if (enemy.health <= 0.0f) {
                    enemy.dying = true;
                    restart(enemy.deathTween);
                    gold += ENEMY_KILL_REWARD_GOLD;
                }
                return true;
            }
            return projectile.traveled > PROJECTILE_MAX_TRAVEL_DISTANCE;
        });
    }

    // Each building type feeds exactly one resource -- summing per-type counts (not a per-tile
    // scan) keeps this O(building types) regardless of world size.
    void updateResources(double deltaTime)
    {
        const float dt = static_cast<float>(deltaTime);
        for (BuildingType type : {BuildingType::goldMine, BuildingType::sawmill, BuildingType::farm}) {
            const BuildingInfo& info = getBuildingInfo(type);
            const int count = world.buildingCounts[static_cast<size_t>(type)];
            gold += count * info.goldPerSecond * dt;
            wood += count * info.woodPerSecond * dt;
            food += count * info.foodPerSecond * dt;
        }
    }

    // Number keys 1/2/3 switch which building type the next left click will place. Left click
    // spends gold to place the selected type on the tile under the cursor (only if that tile is
    // empty and affordable); right click removes whatever is there for free -- one-shot per frame
    // via isKeyPressed()/isMouseButtonPressed(), so holding a key/button doesn't spam actions.
    void handleBuildInput()
    {
        if (isKeyPressed(Key::Num1)) selectedBuildingType = BuildingType::goldMine;
        if (isKeyPressed(Key::Num2)) selectedBuildingType = BuildingType::sawmill;
        if (isKeyPressed(Key::Num3)) selectedBuildingType = BuildingType::farm;

        const bool placeRequested = isMouseButtonPressed(MouseButton::Left);
        const bool removeRequested = isMouseButtonPressed(MouseButton::Right);
        if (!placeRequested && !removeRequested) {
            return;
        }

        if (placeRequested && trySelectHotbarSlot()) {
            return; // clicked a hotbar slot -- that's a selection, not a world placement
        }

        const float2 mouseScreen {static_cast<float>(getMouseX()), static_cast<float>(getMouseY())};
        const float2 mouseWorld = camera.screenToWorld(mouseScreen);
        const int tileX = static_cast<int>(std::floor(mouseWorld.x / TILE_SIZE));
        const int tileY = static_cast<int>(std::floor(mouseWorld.y / TILE_SIZE));

        if (placeRequested) {
            const BuildingInfo& info = getBuildingInfo(selectedBuildingType);
            if (world.buildingTypeAt(tileX, tileY) == BuildingType::none && gold >= info.cost) {
                world.setBuilding(tileX, tileY, selectedBuildingType);
                gold -= info.cost;
                placeSound.play();
            }
        } else if (removeRequested) {
            world.setBuilding(tileX, tileY, BuildingType::none);
        }
    }

    // Wood buys weapon damage, food buys max health -- the first real thing to spend either
    // resource on, and it directly counters the escalating waves. Cost scales with level so
    // stacking one upgrade forever gets steadily less efficient than diversifying.
    void handleUpgradeInput()
    {
        if (isKeyPressed(Key::E)) {
            const float cost = WEAPON_UPGRADE_BASE_COST * static_cast<float>(weaponLevel + 1);
            if (wood >= cost) {
                wood -= cost;
                ++weaponLevel;
                projectileDamage += WEAPON_UPGRADE_DAMAGE_BONUS;
                upgradeSound.play();
            }
        }

        if (isKeyPressed(Key::Q)) {
            const float cost = VITALITY_UPGRADE_BASE_COST * static_cast<float>(vitalityLevel + 1);
            if (food >= cost) {
                food -= cost;
                ++vitalityLevel;
                maxPlayerHealth += VITALITY_UPGRADE_HEALTH_BONUS;
                playerHealth += VITALITY_UPGRADE_HEALTH_BONUS; // heal by the same amount
                upgradeSound.play();
            }
        }
    }

    TileRange computeVisibleTileRange() const
    {
        const float2 topLeftWorld = camera.position - float2 {getWidth() * 0.5f, getHeight() * 0.5f};

        TileRange range {};
        range.startX = constrain(std::floor(topLeftWorld.x / TILE_SIZE) - 1.0f, 0.0f, static_cast<float>(world.width));
        range.startY = constrain(std::floor(topLeftWorld.y / TILE_SIZE) - 1.0f, 0.0f, static_cast<float>(world.height));
        range.endX = constrain(range.startX + std::ceil(getWidth() / TILE_SIZE) + 2.0f, 0.0f, static_cast<float>(world.width));
        range.endY = constrain(range.startY + std::ceil(getHeight() / TILE_SIZE) + 2.0f, 0.0f, static_cast<float>(world.height));
        return range;
    }

    // Sparse, cheap terrain accents scattered over grass at generation time (see
    // World::generateGrassDecorations) -- iterating the whole list and bounds-checking each point
    // is fine at this scale (typically a few hundred), no per-tile lookup needed.
    void drawGrassDecorations(const TileRange& range)
    {
        const float minX = range.startX * TILE_SIZE;
        const float minY = range.startY * TILE_SIZE;
        const float maxX = range.endX * TILE_SIZE;
        const float maxY = range.endY * TILE_SIZE;

        noStroke();
        fill(GRASS_DECORATION_COLOR);
        for (const float2& decoration : world.grassDecorations) {
            if (decoration.x < minX || decoration.x >= maxX || decoration.y < minY || decoration.y >= maxY) {
                continue;
            }
            const float2 screenPosition = camera.worldToScreen(decoration);
            circle(screenPosition.x, screenPosition.y, GRASS_DECORATION_RADIUS);
        }
    }

    void drawVisiblePathTiles(const TileRange& range)
    {
        noStroke();
        for (int y = range.startY; y < range.endY; ++y) {
            for (int x = range.startX; x < range.endX; ++x) {
                if (world.at(x, y) != TileType::path) {
                    continue;
                }

                // Deterministic per-tile shade jitter so the road reads as a textured surface
                // instead of one flat slab of color.
                const int jitter = static_cast<int>(hashTile(x, y) % 17) - 8;
                fill(rgba(getRed(PATH_COLOR) + jitter, getGreen(PATH_COLOR) + jitter, getBlue(PATH_COLOR) + jitter));

                const float2 screenPosition = camera.worldToScreen(float2 {x * TILE_SIZE, y * TILE_SIZE});
                rect(screenPosition.x, screenPosition.y, TILE_SIZE, TILE_SIZE);
            }
        }
    }

    void drawVisibleBuildings(const TileRange& range)
    {
        for (int y = range.startY; y < range.endY; ++y) {
            for (int x = range.startX; x < range.endX; ++x) {
                const BuildingType type = world.buildingTypeAt(x, y);
                if (type == BuildingType::none) {
                    continue;
                }

                const float2 screenPosition = camera.worldToScreen(float2 {x * TILE_SIZE, y * TILE_SIZE});
                const float2 center = screenPosition + float2 {TILE_SIZE * 0.5f, TILE_SIZE * 0.5f};
                const float popScale = world.buildingScaleAt(x, y); // 0 -> 1 with a slight overshoot (easeOutBack)

                withMatrix([&] {
                    translate(center.x, center.y);
                    scale(popScale, popScale);
                    drawBuildingIcon(type);
                });
            }
        }
        noStroke();
    }

    // Each icon is drawn in tile-centered local coordinates (roughly +/- TILE_SIZE/2) so the
    // placement "pop" animation can scale it uniformly around its own center via the matrix
    // stack above, without any icon needing to know about that animation itself.
    void drawBuildingIcon(BuildingType type)
    {
        const color_t bodyColor = getBuildingInfo(type).color;
        switch (type) {
            case BuildingType::goldMine: drawGoldMineIcon(bodyColor); break;
            case BuildingType::sawmill: drawSawmillIcon(bodyColor); break;
            case BuildingType::farm: drawFarmIcon(bodyColor); break;
            default: break;
        }
    }

    // A mound with a dark entrance and a couple of ore sparkles -- distinguishes itself from the
    // other two types by silhouette (mountain triangle), not just color.
    void drawGoldMineIcon(color_t bodyColor)
    {
        const float half = TILE_SIZE * 0.5f - 2.0f;

        noStroke();
        fill(rgba(0, 0, 0, 45));
        ellipse(1.5f, half * 0.75f, half * 0.9f, half * 0.35f);

        fill(bodyColor);
        stroke(darken(bodyColor, 0.45f));
        strokeWeight(1.0f);
        triangle(-half, half * 0.7f, 0.0f, -half, half, half * 0.7f);

        noStroke();
        fill(rgba(40, 32, 22));
        rect(-half * 0.22f, half * 0.15f, half * 0.44f, half * 0.55f, BorderRadius {CornerRadius::circular(half * 0.2f), CornerRadius::circular(half * 0.2f), CornerRadius::circular(0.0f), CornerRadius::circular(0.0f)});

        fill(rgba(255, 223, 90));
        circle(-half * 0.45f, half * 0.1f, 3.0f);
        circle(half * 0.4f, -half * 0.05f, 2.5f);
    }

    // A house silhouette (body + roof) with a circular saw-blade accent instead of a plain block.
    void drawSawmillIcon(color_t bodyColor)
    {
        const float half = TILE_SIZE * 0.5f - 2.0f;

        noStroke();
        fill(rgba(0, 0, 0, 45));
        ellipse(1.5f, half * 0.8f, half * 1.6f, half * 0.35f);

        fill(bodyColor);
        stroke(darken(bodyColor, 0.5f));
        strokeWeight(1.0f);
        rect(-half * 0.75f, -half * 0.1f, half * 1.5f, half * 1.0f);

        noStroke();
        fill(darken(bodyColor, 0.65f));
        triangle(-half * 0.9f, -half * 0.1f, 0.0f, -half, half * 0.9f, -half * 0.1f);

        fill(rgba(220, 220, 225));
        circle(0.0f, half * 0.35f, half * 0.5f);
        stroke(rgba(120, 120, 125));
        strokeWeight(1.0f);
        line(-half * 0.22f, half * 0.35f, half * 0.22f, half * 0.35f);
        line(0.0f, half * 0.13f, 0.0f, half * 0.57f);
    }

    // A soil plot with a small grid of crop-sprout triangles instead of a plain block.
    void drawFarmIcon(color_t bodyColor)
    {
        const float half = TILE_SIZE * 0.5f - 2.0f;
        const BorderRadius rounding = BorderRadius::all(3.0f);

        noStroke();
        fill(rgba(0, 0, 0, 40));
        rect(-half + 1.0f, -half + 3.0f, half * 2.0f, half * 2.0f, rounding);

        fill(rgba(110, 84, 46));
        stroke(rgba(70, 52, 26));
        strokeWeight(1.0f);
        rect(-half, -half, half * 2.0f, half * 2.0f, rounding);

        noStroke();
        fill(bodyColor);
        for (int row = 0; row < 2; ++row) {
            for (int col = 0; col < 3; ++col) {
                const float sproutX = -half * 0.55f + col * half * 0.55f;
                const float sproutY = -half * 0.4f + row * half * 0.75f;
                triangle(sproutX, sproutY + 4.0f, sproutX - 3.0f, sproutY - 4.0f, sproutX + 3.0f, sproutY - 4.0f);
            }
        }
    }

    // Drawn as a single oriented dart/arrow polygon (rotated to face movement direction) instead
    // of a plain circle -- a walk bob while moving and a stretch + tint while dashing make it
    // read as an animated character rather than a static token.
    void drawPlayer()
    {
        // Blink while invulnerable (a few frames on, a few off) instead of just rendering solid --
        // makes the post-hit grace window actually readable instead of a silent damage-immune period.
        if (playerInvulnerabilityTimer > 0.0f && std::fmod(playerInvulnerabilityTimer, 0.2f) < 0.1f) {
            return;
        }

        const float bob = std::sin(player.animationTime * PLAYER_BOB_FREQUENCY) * PLAYER_BOB_AMPLITUDE;
        const float2 screenPosition = camera.worldToScreen(player.position) + float2 {0.0f, bob};
        const float angle = std::atan2(player.facing.y, player.facing.x);
        const float noseLength = PLAYER_RADIUS + (player.isDashing ? 12.0f : 5.0f);

        withMatrix([&] {
            translate(screenPosition.x, screenPosition.y);
            rotate(angle);

            noStroke();
            fill(player.isDashing ? PLAYER_DASH_COLOR : PLAYER_COLOR);
            beginShape();
            vertex(noseLength, 0.0f);
            vertex(-PLAYER_RADIUS * 0.6f, PLAYER_RADIUS);
            vertex(-PLAYER_RADIUS * 0.2f, 0.0f); // concave notch at the back -- gives it a dart-like silhouette
            vertex(-PLAYER_RADIUS * 0.6f, -PLAYER_RADIUS);
            endShape(true);
        });
    }

    // A brief fading line from the player to the target hit -- the projectile itself is tiny and
    // easy to miss at a glance, this makes every shot register visually the instant it fires.
    void drawMuzzleFlash()
    {
        if (muzzleFlashTimer <= 0.0f) {
            return;
        }

        const float alpha = muzzleFlashTimer / MUZZLE_FLASH_DURATION;
        const float2 from = camera.worldToScreen(player.position);
        const float2 to = camera.worldToScreen(muzzleFlashTarget);

        noFill();
        stroke(rgba(getRed(MUZZLE_FLASH_COLOR), getGreen(MUZZLE_FLASH_COLOR), getBlue(MUZZLE_FLASH_COLOR), static_cast<int>(alpha * 200.0f)));
        strokeWeight(2.0f);
        line(from.x, from.y, to.x, to.y);
        noStroke();
    }

    // Enemies/projectiles aren't culled to the visible range like tiles are -- their counts stay
    // small (a handful per wave), so it isn't worth the extra bookkeeping yet.
    void drawEnemies()
    {
        for (const Enemy& enemy : enemies) {
            const float2 screenPosition = camera.worldToScreen(enemy.position);

            if (enemy.dying) {
                const float scale = value(enemy.deathTween); // eases 1 -> 0 over ENEMY_DEATH_DURATION
                noStroke();
                fill(rgba(getRed(ENEMY_COLOR), getGreen(ENEMY_COLOR), getBlue(ENEMY_COLOR), static_cast<int>(scale * 255.0f)));
                circle(screenPosition.x, screenPosition.y, ENEMY_RADIUS * scale);
                continue;
            }

            const float angle = std::atan2(enemy.facing.y, enemy.facing.x);
            withMatrix([&] {
                translate(screenPosition.x, screenPosition.y);
                rotate(angle);

                noStroke();
                if (enemy.hitFlashTimer > 0.0f) {
                    fill(rgba(255));
                    circle(0.0f, 0.0f, ENEMY_RADIUS);
                } else {
                    drawEnemyIcon();
                }
            });
        }
    }

    // A small imp-like creature (elongated body, backswept side horns, glowing forward eyes)
    // oriented toward the player -- drawn in local space with "forward" along +X, matching the
    // rotation applied by the caller.
    void drawEnemyIcon()
    {
        const float r = ENEMY_RADIUS;

        fill(ENEMY_COLOR);
        ellipse(0.0f, 0.0f, r * 1.15f, r * 0.95f);

        fill(darken(ENEMY_COLOR, 0.6f));
        triangle(-r * 0.2f, -r * 0.65f, -r * 1.1f, -r * 1.1f, -r * 0.55f, -r * 0.2f);
        triangle(-r * 0.2f, r * 0.65f, -r * 1.1f, r * 1.1f, -r * 0.55f, r * 0.2f);

        fill(rgba(255, 225, 60));
        circle(r * 0.45f, -r * 0.28f, 2.2f);
        circle(r * 0.45f, r * 0.28f, 2.2f);
    }

    // Small arrows instead of plain dots -- shaft, fletching, and a bright arrowhead, oriented
    // along the projectile's actual flight direction.
    void drawProjectiles()
    {
        for (const Projectile& projectile : projectiles) {
            const float2 screenPosition = camera.worldToScreen(projectile.position);
            const float angle = std::atan2(projectile.velocity.y, projectile.velocity.x);

            withMatrix([&] {
                translate(screenPosition.x, screenPosition.y);
                rotate(angle);

                noStroke();
                fill(PROJECTILE_SHAFT_COLOR);
                rect(-7.0f, -1.0f, 10.0f, 2.0f);
                triangle(-7.0f, 0.0f, -4.0f, -2.5f, -4.0f, 0.0f);
                triangle(-7.0f, 0.0f, -4.0f, 2.5f, -4.0f, 0.0f);

                fill(PROJECTILE_HEAD_COLOR);
                triangle(7.0f, 0.0f, 2.0f, -3.0f, 2.0f, 3.0f);
            });
        }
    }

    // Screen-space overlay -- drawn last, in screen coordinates, unaffected by the camera.
    // Five focused panels instead of one dense text block: resources (top-left), HP (top-center,
    // as requested), stats (top-right), building hotbar (bottom-center), control hints (corner).
    void drawHud()
    {
        drawResourcePanel();
        drawHealthBar();
        drawStatsPanel();
        drawBuildingHotbar();
        drawControlHints();
    }

    // Shared parchment/wood panel look -- a soft shadow, a beige body, a brown border -- so every
    // HUD element reads as part of one coordinated theme instead of ad hoc boxes.
    void drawHudPanel(float x, float y, float w, float h)
    {
        noStroke();
        fill(rgba(0, 0, 0, 45));
        rect(x + 2.0f, y + 3.0f, w, h, BorderRadius::all(8.0f));

        fill(HUD_PANEL_COLOR);
        stroke(HUD_PANEL_BORDER_COLOR);
        strokeWeight(2.0f);
        rect(x, y, w, h, BorderRadius::all(8.0f));
    }

    void drawResourcePanel()
    {
        constexpr float x = 16.0f;
        constexpr float y = 16.0f;
        constexpr float w = 210.0f;
        constexpr float h = 84.0f;
        drawHudPanel(x, y, w, h);

        const struct
        {
            color_t swatch;
            const char* label;
            int amount;
        } rows[] = {
            {getBuildingInfo(BuildingType::goldMine).color, "Gold", static_cast<int>(gold)},
            {getBuildingInfo(BuildingType::sawmill).color, "Wood", static_cast<int>(wood)},
            {getBuildingInfo(BuildingType::farm).color, "Food", static_cast<int>(food)},
        };

        textAlign(TextAlignment::topLeft);
        textSize(15.0f);
        for (int i = 0; i < 3; ++i) {
            const float rowY = y + 14.0f + i * 22.0f;
            noStroke();
            fill(rows[i].swatch);
            circle(x + 18.0f, rowY + 6.0f, 9.0f);
            fill(HUD_TEXT_COLOR);
            text(std::format("{}: {}", rows[i].label, rows[i].amount), x + 34.0f, rowY);
        }
    }

    // Moved to top-center per request -- green/yellow/red fill fraction instead of a bare number,
    // readable at a glance during combat.
    void drawHealthBar()
    {
        constexpr float w = 300.0f;
        constexpr float h = 26.0f;
        const float x = (getWidth() - w) * 0.5f;
        constexpr float y = 16.0f;
        drawHudPanel(x - 4.0f, y - 4.0f, w + 8.0f, h + 8.0f);

        const float ratio = constrain(playerHealth / maxPlayerHealth, 0.0f, 1.0f);
        const color_t fillColor = ratio > 0.5f ? rgba(90, 170, 70) : ratio > 0.25f ? rgba(210, 165, 50)
                                                                                   : rgba(190, 60, 50);
        const BorderRadius rounding = BorderRadius::all(6.0f);

        noStroke();
        fill(darken(HUD_PANEL_BORDER_COLOR, 0.7f));
        rect(x, y, w, h, rounding);

        // A *rounded* rect's texCoords are computed as (x-left)/width -- at exactly 0 HP,
        // w * ratio is exactly 0.0f, making that a 0/0 division (NaN), which is exactly what
        // tesselate_polygon's "non-finite vertex position, texCoord, or color" was rejecting.
        // Skip the fill entirely rather than ever asking for a zero-width rounded rect.
        if (ratio > 0.0f) {
            fill(fillColor);
            rect(x, y, w * ratio, h, rounding);
        }

        fill(HUD_TEXT_COLOR);
        textAlign(TextAlignment::center);
        textSize(16.0f);
        text(std::format("HP {}/{}", std::max(0, static_cast<int>(playerHealth)), static_cast<int>(maxPlayerHealth)), x + w * 0.5f, y + h * 0.5f);
    }

    void drawStatsPanel()
    {
        constexpr float w = 230.0f;
        constexpr float h = 84.0f;
        const float x = getWidth() - w - 16.0f;
        constexpr float y = 16.0f;
        drawHudPanel(x, y, w, h);

        const float nextWeaponCost = WEAPON_UPGRADE_BASE_COST * static_cast<float>(weaponLevel + 1);
        const float nextVitalityCost = VITALITY_UPGRADE_BASE_COST * static_cast<float>(vitalityLevel + 1);

        textAlign(TextAlignment::topLeft);
        textSize(15.0f);
        fill(HUD_TEXT_COLOR);
        text(std::format("Wave {}   Enemies {}", waveNumber, enemies.size()), x + 14.0f, y + 12.0f);

        noStroke();
        fill(getBuildingInfo(BuildingType::sawmill).color);
        circle(x + 20.0f, y + 42.0f, 8.0f);
        fill(HUD_TEXT_COLOR);
        text(std::format("Weapon Lv{} -- E: {}w", weaponLevel, static_cast<int>(nextWeaponCost)), x + 34.0f, y + 36.0f);

        fill(getBuildingInfo(BuildingType::farm).color);
        circle(x + 20.0f, y + 64.0f, 8.0f);
        fill(HUD_TEXT_COLOR);
        text(std::format("Vitality Lv{} -- Q: {}f", vitalityLevel, static_cast<int>(nextVitalityCost)), x + 34.0f, y + 58.0f);
    }

    // Same 3 slot rects used both to draw the hotbar and to hit-test clicks against it, so the
    // two can never drift out of sync.
    std::array<rect2f, 3> computeHotbarSlotRects() const
    {
        constexpr float slotSize = 64.0f;
        constexpr float gap = 10.0f;
        constexpr float totalWidth = 3.0f * slotSize + 2.0f * gap;
        const float startX = (getWidth() - totalWidth) * 0.5f;
        const float y = getHeight() - slotSize - 20.0f;

        std::array<rect2f, 3> slots {};
        for (int i = 0; i < 3; ++i) {
            slots[i] = rect2f {startX + i * (slotSize + gap), y, slotSize, slotSize};
        }
        return slots;
    }

    static std::array<BuildingType, 3> hotbarBuildingTypes()
    {
        return {BuildingType::goldMine, BuildingType::sawmill, BuildingType::farm};
    }

    // A static icon hotbar (always visible, no opening/closing) -- selecting a building this way
    // doesn't interrupt combat the way a popup dropdown menu would.
    void drawBuildingHotbar()
    {
        static const std::array<const char*, 3> hotkeys {"1", "2", "3"};
        const std::array<BuildingType, 3> types = hotbarBuildingTypes();
        const std::array<rect2f, 3> slots = computeHotbarSlotRects();

        for (size_t i = 0; i < slots.size(); ++i) {
            const rect2f& slot = slots[i];
            const BuildingType type = types[i];
            const BuildingInfo& info = getBuildingInfo(type);
            const bool selected = type == selectedBuildingType;
            const BorderRadius rounding = BorderRadius::all(10.0f);

            noStroke();
            fill(rgba(0, 0, 0, 45));
            rect(slot.left + 2.0f, slot.top + 3.0f, slot.width, slot.height, rounding);

            fill(HUD_PANEL_COLOR);
            stroke(selected ? HUD_SELECTED_COLOR : HUD_PANEL_BORDER_COLOR);
            strokeWeight(selected ? 3.0f : 2.0f);
            rect(slot.left, slot.top, slot.width, slot.height, rounding);

            const float iconCenterX = slot.left + slot.width * 0.5f;
            const float iconCenterY = slot.top + slot.height * 0.42f;
            withMatrix([&] {
                translate(iconCenterX, iconCenterY);
                scale(0.85f, 0.85f);
                drawBuildingIcon(type);
            });

            noStroke();
            fill(HUD_TEXT_COLOR);
            textAlign(TextAlignment::bottomCenter);
            textSize(12.0f);
            text(std::format("{}  {}g", hotkeys[i], static_cast<int>(info.cost)), slot.left + slot.width * 0.5f, slot.top + slot.height - 4.0f);
        }
    }

    // Returns true (and applies the selection) if the given click landed on a hotbar slot.
    bool trySelectHotbarSlot()
    {
        const std::array<BuildingType, 3> types = hotbarBuildingTypes();
        const std::array<rect2f, 3> slots = computeHotbarSlotRects();
        const float mouseX = static_cast<float>(getMouseX());
        const float mouseY = static_cast<float>(getMouseY());

        for (size_t i = 0; i < slots.size(); ++i) {
            const rect2f& slot = slots[i];
            if (mouseX >= slot.left && mouseX < slot.left + slot.width && mouseY >= slot.top && mouseY < slot.top + slot.height) {
                selectedBuildingType = types[i];
                return true;
            }
        }
        return false;
    }

    // Small, muted, sized to the actual text -- a hint rather than another dense panel.
    void drawControlHints()
    {
        const std::string hint = "WASD/Arrows: move   Shift: dash   LMB: build/select   RMB: remove";

        textAlign(TextAlignment::center);
        textSize(13.0f);
        const float w = textWidth(hint) + 24.0f;
        constexpr float h = 26.0f;
        const float x = getWidth() - w - 16.0f;
        const float y = getHeight() - h - 16.0f;

        noStroke();
        fill(rgba(0, 0, 0, 120));
        rect(x, y, w, h, BorderRadius::all(6.0f));

        fill(HUD_TEXT_MUTED_COLOR);
        text(hint, x + w * 0.5f, y + h * 0.5f);
    }

    // A themed, clickable button -- returns true the one frame it's clicked. Reuses the hotbar's
    // selection color for the hover highlight, so it reads as part of the same UI system.
    bool drawButton(const rect2f& bounds, std::string_view label)
    {
        const float mouseX = static_cast<float>(getMouseX());
        const float mouseY = static_cast<float>(getMouseY());
        const bool hovered = mouseX >= bounds.left && mouseX < bounds.left + bounds.width && mouseY >= bounds.top && mouseY < bounds.top + bounds.height;
        const bool clicked = hovered && isMouseButtonPressed(MouseButton::Left);
        const BorderRadius rounding = BorderRadius::all(8.0f);

        noStroke();
        fill(rgba(0, 0, 0, 60));
        rect(bounds.left + 2.0f, bounds.top + 3.0f, bounds.width, bounds.height, rounding);

        fill(hovered ? HUD_SELECTED_COLOR : HUD_PANEL_COLOR);
        stroke(HUD_PANEL_BORDER_COLOR);
        strokeWeight(2.0f);
        rect(bounds.left, bounds.top, bounds.width, bounds.height, rounding);

        noStroke();
        fill(HUD_TEXT_COLOR);
        textAlign(TextAlignment::center);
        textSize(20.0f);
        text(label, bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);

        return clicked;
    }

    // The frozen game world (already generated by the resetGame() call in setup()) as a dimmed
    // backdrop behind the menu panel -- the same trick used for the game-over screen.
    void drawMainMenuScreen()
    {
        renderFrame();

        noStroke();
        fill(rgba(0, 0, 0, 100));
        rect(0.0f, 0.0f, getWidth(), getHeight());

        constexpr float panelW = 420.0f;
        constexpr float panelH = 260.0f;
        const float panelX = (getWidth() - panelW) * 0.5f;
        const float panelY = (getHeight() - panelH) * 0.5f;
        drawHudPanel(panelX, panelY, panelW, panelH);

        fill(HUD_TEXT_COLOR);
        textAlign(TextAlignment::center);
        textSize(38.0f);
        text("Kingshot Clone", getWidth() * 0.5f, panelY + 56.0f);

        fill(HUD_TEXT_SUBTLE_COLOR);
        textSize(15.0f);
        text("Build an economy, survive the waves.", getWidth() * 0.5f, panelY + 92.0f);

        const rect2f playButton {getWidth() * 0.5f - 90.0f, panelY + 128.0f, 180.0f, 52.0f};
        if (drawButton(playButton, "Play")) {
            resetGame();
            state = GameState::playing;
        }

        fill(HUD_TEXT_SUBTLE_COLOR);
        textSize(13.0f);
        text("WASD/Arrows: move -- Shift: dash -- LMB: build -- RMB: remove", getWidth() * 0.5f, panelY + panelH - 22.0f);
    }

    void drawGameOverScreen()
    {
        noStroke();
        fill(rgba(0, 0, 0, 170));
        rect(0.0f, 0.0f, getWidth(), getHeight());

        constexpr float panelW = 360.0f;
        constexpr float panelH = 232.0f;
        const float panelX = (getWidth() - panelW) * 0.5f;
        const float panelY = (getHeight() - panelH) * 0.5f;
        drawHudPanel(panelX, panelY, panelW, panelH);

        fill(GAME_OVER_TEXT_COLOR);
        textAlign(TextAlignment::center);
        textSize(30.0f);
        text("Game Over", getWidth() * 0.5f, panelY + 44.0f);

        fill(HUD_TEXT_SUBTLE_COLOR);
        textSize(16.0f);
        text(std::format("Survived {} wave{}", waveNumber, waveNumber == 1 ? "" : "s"), getWidth() * 0.5f, panelY + 78.0f);

        const rect2f restartButton {getWidth() * 0.5f - 90.0f, panelY + 108.0f, 180.0f, 46.0f};
        const bool restartClicked = drawButton(restartButton, "Restart");
        if (restartClicked || isKeyPressed(Key::R)) {
            resetGame();
            state = GameState::playing;
        }

        const rect2f menuButton {getWidth() * 0.5f - 90.0f, panelY + 164.0f, 180.0f, 46.0f};
        if (drawButton(menuButton, "Main Menu")) {
            state = GameState::mainMenu;
        }
    }
};

SketchSpec p5::createSpec()
{
    return {
        .plugins = [] {
            std::vector<std::unique_ptr<Plugin>> plugins;
            plugins.push_back(createAudioPlugin());
            return plugins;
        },
        .sketch = [] {
            return std::make_unique<KingshotClone>();
        },
    };
}
