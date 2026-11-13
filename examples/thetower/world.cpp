#include "world.hpp"

#include <algorithm>
#include <format>

#include "arena.hpp"
#include "enemy_roster.hpp"
#include "palette.hpp"

using namespace p5;

namespace
{
    constexpr float speedStep = 0.25f;

    constexpr float shakeDecay = 7.0f;
    constexpr float maxShake = 18.0f;
    constexpr float gameOverFade = 0.8f;

    void shake_add(World& world, const float strength)
    {
        world.shakeStrength = std::min(world.shakeStrength + strength, maxShake);
    }

    // strikes on the tower and dying enemies shove the playfield, the heavier the stronger
    void shake_from_events(World& world)
    {
        for (const TowerHitEvent& hit : world.events.towerHits) {
            shake_add(world, std::min(hit.damage * 0.3f, 6.0f));
        }
        for (const DeathEvent& death : world.events.deaths) {
            shake_add(world, death.size * 0.05f);
        }
    }

    // everything that only fades out, it keeps running once the round is over
    void update_effects(World& world, const float deltaTime)
    {
        world.shakeStrength = world.shakeStrength < 0.05f ? 0.0f : damp(world.shakeStrength, 0.0f, shakeDecay, deltaTime);
        world.shake = random2D(world.shakeStrength);

        death_effects_update(world.deathEffects, deltaTime);
        hit_sparks_update(world.hitSparks, deltaTime);
        floating_texts_update(world.floatingTexts, deltaTime);
    }

    // every enemy strike first reflects thorns onto the attacker, then the defense reduces what reaches the tower
    void resolve_tower_hits(World& world, const TowerStats& stats)
    {
        for (const TowerHitEvent& hit : world.events.towerHits) {
            // thorns use the damage before any defense, so both stats stay worth buying together
            if (stats.thorns > 0.0f) {
                if (Enemy* attacker = enemy_find(world.enemies, hit.attacker)) {
                    const Hit thornsHit = {
                        .damage = hit.damage * stats.thorns,
                        .isCrit = false,
                        .reserved = false,
                        .bouncesLeft = 0,
                        .direction = {},
                        .knockback = 0.0f,
                        .slowFactor = 0.0f,
                        .slowDuration = 0.0f,
                    };
                    enemy_core_apply_hit(attacker->core, thornsHit, world.events);
                }
            }

            const float damage = std::max(hit.damage * (1.0f - stats.percentDefense) - stats.absoluteDefense, 0.0f);
            if (damage > 0.0f) {
                tower_damage(world.tower, damage);
            }
        }
    }

    // a type that falls apart leaves its children behind, they come in around where it died
    void spawn_death_children(World& world)
    {
        for (const DeathEvent& death : world.events.deaths) {
            const DeathSpawn& spawn = death.spawn;
            if (spawn.factory == nullptr) {
                continue;
            }

            for (int i = 0; i < spawn.count; ++i) {
                const float2 offset = fromAngle(degrees(360.0f / static_cast<float>(spawn.count) * static_cast<float>(i)), spawn.spread);
                world.enemies.push_back(spawn.factory(world.nextEnemyId++, death.position + offset));
            }
        }
    }

    void show_crit_numbers(World& world)
    {
        for (const DamageEvent& damage : world.events.damage) {
            // only crits get a number, a popup for every hit would drown out everything else
            if (damage.isCrit) {
                world.floatingTexts.push_back(floating_text_create(std::format("{:.0f}!", damage.amount), damage.position + float2 {0.0f, -12.0f}, critColor, 15.0f, 55.0f, 0.6f));
            }
        }
    }

    void collect_kill_rewards(World& world, const TowerStats& stats)
    {
        for (const DeathEvent& death : world.events.deaths) {
            const int reward = death.goldReward + stats.goldPerKill;
            world.gold += reward;
            world.floatingTexts.push_back(floating_text_create(std::format("+{}", reward), death.position, goldColor, std::min(11.0f + 1.2f * reward, 20.0f), 45.0f, 0.8f));

            // coins per kill are fractional, a coin pops once enough fractions have added up
            world.coinFraction += stats.coinsPerKill;
            if (world.coinFraction >= 1.0f) {
                const int earned = static_cast<int>(world.coinFraction);
                world.coinFraction -= static_cast<float>(earned);
                world.coins += earned;
                world.floatingTexts.push_back(floating_text_create(std::format("+{} C", earned), death.position + float2 {0.0f, 16.0f}, coinColor, 12.0f, 40.0f, 1.0f));
            }
        }
    }

    void collect_wave_rewards(World& world, const TowerStats& stats)
    {
        for (const WaveCompletedEvent& wave : world.events.wavesCompleted) {
            world.gold += stats.goldPerWave;
            world.coins += stats.coinsPerWave;

            world.floatingTexts.push_back(floating_text_create(std::format("Welle {} geschafft", wave.wave), world.tower.position + float2 {0.0f, -84.0f}, rgba(230), 20.0f, 16.0f, 2.0f));
            world.floatingTexts.push_back(floating_text_create(std::format("+{} G", stats.goldPerWave), world.tower.position + float2 {0.0f, -60.0f}, goldColor, 14.0f, 16.0f, 2.0f));
            world.floatingTexts.push_back(floating_text_create(std::format("+{} C", stats.coinsPerWave), world.tower.position + float2 {0.0f, -42.0f}, coinColor, 13.0f, 16.0f, 2.0f));
        }
    }
}

World world_create(const UpgradeLevels& startLevels)
{
    const float2 center = {300.0f, 300.0f};

    WaveConfig waveConfig = {
        .center = center,
        .spawnDistance = 280.0f,
        .budgetBase = 8,
        .budgetGrowthPerWave = 3,
        .waveDuration = 20.0f,
        .bossWaveInterval = 10,
        .enemies = {enemy_roster().begin(), enemy_roster().end()},
        .bosses = {},
    };
    WaveSpawner waves = wave_spawner_create(waveConfig);

    return World {
        .tower = tower_create(center, 50.0f),
        .weapon = TowerWeapon {
            .cooldown = 0.0f,
            .rapidFireLeft = 0.0f,
            .shotFlash = p5::animation::valueTweenJump(0.0f),
        },
        .baseStats = tower_stats_base(),
        .startLevels = startLevels,
        .upgrades = startLevels,
        .gold = 0,
        .coins = 0,
        .coinFraction = 0.0f,
        .waveConfig = std::move(waveConfig),
        .waves = std::move(waves),
        .nextEnemyId = 0,
        .enemies = {},
        .bullets = {},
        .enemyShots = {},
        .deathEffects = {},
        .hitSparks = {},
        .floatingTexts = {},
        .shake = {},
        .shakeStrength = 0.0f,
        .speed = tower_stats_compute(tower_stats_base(), startLevels).gameSpeed,
        .events = {},
        .gameOver = false,
        .gameOverTime = 0.0f,
    };
}

TowerStats world_tower_stats(const World& world)
{
    return tower_stats_compute(world.baseStats, world.upgrades);
}

float world_speed(const World& world)
{
    return std::clamp(world.speed, worldMinSpeed, world_tower_stats(world).gameSpeed);
}

void world_change_speed(World& world, const int steps)
{
    world.speed = world_speed(world) + speedStep * static_cast<float>(steps);
    world.speed = world_speed(world);
}

bool world_buy_upgrade(World& world, const UpgradeDefinition& definition)
{
    const float maxSpeedBefore = world_tower_stats(world).gameSpeed;
    if (!upgrade_buy_with_gold(definition, world.upgrades, world.startLevels, world.gold)) {
        return false;
    }

    // whoever plays at full speed wants the newly unlocked speed right away
    if (world_speed(world) >= maxSpeedBefore) {
        world.speed = world_tower_stats(world).gameSpeed;
    }
    return true;
}

void world_update(World& world, const float realDeltaTime)
{
    const TowerStats stats = world_tower_stats(world);
    const float deltaTime = realDeltaTime * world_speed(world);

    if (world.gameOver) {
        world.gameOverTime += realDeltaTime;
        update_effects(world, deltaTime);
        return;
    }

    game_events_clear(world.events);
    wave_spawner_update(world.waves, world.waveConfig, world.enemies, world.nextEnemyId, world.events, deltaTime);

    for (Enemy& enemy : world.enemies) {
        enemy_update(enemy, world.tower, world.events, deltaTime);
    }

    // shots are in the air before the tower resolves what hit it this frame
    enemy_shots_spawn(world.enemyShots, world.tower, world.events);
    enemy_shots_update(world.enemyShots, world.events, deltaTime);

    resolve_tower_hits(world, stats);
    tower_regenerate(world.tower, stats.healthRegen * deltaTime);
    tower_update(world.tower, deltaTime);

    if (tower_is_destroyed(world.tower, stats.maxHealth)) {
        world.gameOver = true;
        // the tower goes out with a burst of its own, the overlay fades in over it
        world.deathEffects.push_back(particle_burst_create(ParticleDeathAnimation {.color = accentColor}, world.tower.position, world.tower.radius * 2.4f));
        shake_add(world, maxShake);
        return;
    }

    bullets_update(world.bullets, world.enemies, stats, world.events, deltaTime);
    show_crit_numbers(world);
    enemies_execute(world.enemies, stats.executeThreshold, world.events);
    enemies_remove_dead(world.enemies);
    spawn_death_children(world);
    collect_kill_rewards(world, stats);
    collect_wave_rewards(world, stats);

    death_effects_spawn(world.deathEffects, world.events);
    hit_sparks_spawn(world.hitSparks, world.events);
    shake_from_events(world);
    update_effects(world, deltaTime);
    tower_weapon_update(world.weapon, stats, world.tower, world.enemies, world.bullets, deltaTime);
}

void world_show(const World& world, const p5::Font& font, const p5::rect2f& view)
{
    const TowerStats stats = world_tower_stats(world);
    const float healthFraction = stats.maxHealth > 0.0f ? tower_health(world.tower, stats.maxHealth) / stats.maxHealth : 0.0f;

    withMatrix([&] {
        translate(world.shake.x, world.shake.y);

        // the shake moves the floor along, so it has to reach a little past the view
        arena_show(rect2f {view.left - maxShake, view.top - maxShake, view.width + maxShake * 2.0f, view.height + maxShake * 2.0f}, world.tower.position, world.waveConfig.spawnDistance);
        tower_show(world.tower, stats.range, healthFraction);
        tower_weapon_show(world.weapon, stats, world.tower);
        for (const Enemy& enemy : world.enemies) {
            enemy_show(enemy);
        }
        hit_sparks_show(world.hitSparks);
        death_effects_show(world.deathEffects);
        for (const Bullet& bullet : world.bullets) {
            bullet_show(bullet);
        }
        enemy_shots_show(world.enemyShots);
        floating_texts_show(world.floatingTexts, font);
    });

    if (world.gameOver) {
        // the overlay waits for the burst, the text comes once the playfield has gone quiet
        const float fade = std::min(world.gameOverTime / gameOverFade, 1.0f);
        const float textFade = std::clamp((world.gameOverTime - gameOverFade * 0.5f) / gameOverFade, 0.0f, 1.0f);

        with([&] {
            textFont(font);
            textAlign(TextAlignment::center);
            noStroke();

            fill(withOpacity(rgba(26), 0.82f * fade));
            rect(view.left, view.top, view.width, view.height);

            fill(withOpacity(dangerColor, textFade));
            textSize(34.0f);
            text("Turm zerstört", world.tower.position.x, world.tower.position.y - 30.0f);

            fill(withOpacity(rgba(230), textFade));
            textSize(16.0f);
            text(std::format("Welle {} erreicht", world.waves.wave), world.tower.position.x, world.tower.position.y + 8.0f);

            fill(withOpacity(coinColor, textFade));
            textSize(16.0f);
            text(std::format("+{} Coins", world.coins), world.tower.position.x, world.tower.position.y + 34.0f);

            fill(withOpacity(rgba(130), textFade));
            textSize(13.0f);
            text("Klicken für Hauptmenü", world.tower.position.x, world.tower.position.y + 66.0f);
        });
    }
}
