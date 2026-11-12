#include "world.hpp"

#include <algorithm>
#include <format>

using namespace p5;

namespace
{
    constexpr color_t goldColor = rgba(230, 200, 90);
    constexpr color_t coinColor = rgba(180, 140, 240);
    constexpr color_t critColor = rgba(255, 120, 60);

    constexpr float speedStep = 0.25f;
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
        .enemies = {
            SpawnEntry {
                .archetype = &easyEnemy,
                .cost = 1,
                .weight = SpawnWeight {
                    .unlockWave = 1,
                    .baseWeight = 10.0f,
                    .growthPerWave = 0.0f,
                },
            },
            SpawnEntry {
                .archetype = &fastEnemy,
                .cost = 1,
                .weight = SpawnWeight {
                    .unlockWave = 2,
                    .baseWeight = 3.0f,
                    .growthPerWave = 0.5f,
                },
            },
            SpawnEntry {
                .archetype = &tankEnemy,
                .cost = 4,
                .weight = SpawnWeight {
                    .unlockWave = 4,
                    .baseWeight = 1.0f,
                    .growthPerWave = 0.4f,
                },
            },
        },
        .bosses = {},
    };
    WaveSpawner waves = wave_spawner_create(waveConfig);

    return World {
        .tower = tower_create(center, 50.0f),
        .weapon = TowerWeapon {
            .cooldown = 0.0f,
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
        .deathEffects = {},
        .floatingTexts = {},
        .speed = tower_stats_compute(tower_stats_base(), startLevels).gameSpeed,
        .events = {},
        .gameOver = false,
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
    if (world.gameOver) {
        return;
    }

    const TowerStats stats = world_tower_stats(world);
    const float deltaTime = realDeltaTime * world_speed(world);

    game_events_clear(world.events);
    wave_spawner_update(world.waves, world.waveConfig, world.enemies, world.nextEnemyId, world.events, deltaTime);

    for (Enemy& enemy : world.enemies) {
        enemy_update(enemy, world.tower, world.events, deltaTime);
    }

    for (const TowerHitEvent& hit : world.events.towerHits) {
        // thorns use the damage before any defense, so both stats stay worth buying together
        if (stats.thorns > 0.0f) {
            if (Enemy* attacker = enemy_find(world.enemies, hit.attacker)) {
                const Hit thornsHit = {
                    .damage = hit.damage * stats.thorns,
                    .isCrit = false,
                    .reserved = false,
                    .bouncesLeft = 0,
                };
                enemy_core_apply_hit(attacker->core, thornsHit, world.events);
            }
        }

        const float damage = std::max(hit.damage * (1.0f - stats.percentDefense) - stats.absoluteDefense, 0.0f);
        if (damage > 0.0f) {
            tower_damage(world.tower, damage);
        }
    }
    tower_regenerate(world.tower, stats.healthRegen * deltaTime);
    tower_update(world.tower, deltaTime);

    if (tower_is_destroyed(world.tower, stats.maxHealth)) {
        world.gameOver = true;
        return;
    }

    bullets_update(world.bullets, world.enemies, stats, world.events, deltaTime);
    for (const DamageEvent& damage : world.events.damage) {
        // only crits get a number, a popup for every hit would drown out everything else
        if (damage.isCrit) {
            world.floatingTexts.push_back(floating_text_create(std::format("{:.0f}!", damage.amount), damage.position + float2 {0.0f, -14.0f}, critColor, 24.0f, 70.0f, 0.7f));
        }
    }
    enemies_remove_dead(world.enemies);
    for (const DeathEvent& death : world.events.deaths) {
        const int reward = death.goldReward + stats.goldPerKill;
        world.gold += reward;
        world.floatingTexts.push_back(floating_text_create(std::format("+{}", reward), death.position, goldColor, std::min(16.0f + 2.0f * reward, 30.0f), 60.0f, 0.9f));

        // coins per kill are fractional, a coin pops once enough fractions have added up
        world.coinFraction += stats.coinsPerKill;
        if (world.coinFraction >= 1.0f) {
            const int earned = static_cast<int>(world.coinFraction);
            world.coinFraction -= static_cast<float>(earned);
            world.coins += earned;
            world.floatingTexts.push_back(floating_text_create(std::format("+{} C", earned), death.position + float2 {0.0f, 18.0f}, coinColor, 18.0f, 50.0f, 1.1f));
        }
    }
    for (const WaveCompletedEvent& wave : world.events.wavesCompleted) {
        world.gold += stats.goldPerWave;
        world.floatingTexts.push_back(floating_text_create(std::format("Welle {} geschafft", wave.wave), world.tower.position + float2 {0.0f, -90.0f}, rgba(230), 28.0f, 20.0f, 2.0f));
        world.floatingTexts.push_back(floating_text_create(std::format("+{} G", stats.goldPerWave), world.tower.position + float2 {0.0f, -60.0f}, goldColor, 20.0f, 20.0f, 2.0f));

        world.coins += stats.coinsPerWave;
        world.floatingTexts.push_back(floating_text_create(std::format("+{} C", stats.coinsPerWave), world.tower.position + float2 {0.0f, -36.0f}, coinColor, 18.0f, 20.0f, 2.0f));
    }

    death_effects_spawn(world.deathEffects, world.events);
    death_effects_update(world.deathEffects, deltaTime);
    floating_texts_update(world.floatingTexts, deltaTime);
    tower_weapon_update(world.weapon, stats, world.tower, world.enemies, world.bullets, deltaTime);
}

void world_show(const World& world, const p5::Font& font)
{
    tower_show(world.tower, world_tower_stats(world).range);
    for (const Enemy& enemy : world.enemies) {
        enemy_show(enemy);
    }
    death_effects_show(world.deathEffects);
    for (const Bullet& bullet : world.bullets) {
        bullet_show(bullet);
    }
    floating_texts_show(world.floatingTexts, font);

    if (world.gameOver) {
        with([&] {
            textFont(font);
            textAlign(TextAlignment::center);
            noStroke();

            fill(rgba(31, 200));
            rect(-worldSize, -worldSize, worldSize * 3.0f, worldSize * 3.0f);

            fill(rgba(230, 80, 80));
            textSize(40.0f);
            text("Turm zerstört", world.tower.position.x, world.tower.position.y - 30.0f);

            fill(rgba(230));
            textSize(18.0f);
            text(std::format("Welle {} erreicht", world.waves.wave), world.tower.position.x, world.tower.position.y + 10.0f);

            fill(coinColor);
            textSize(18.0f);
            text(std::format("+{} Coins", world.coins), world.tower.position.x, world.tower.position.y + 38.0f);

            fill(rgba(130));
            textSize(14.0f);
            text("Klicken für Hauptmenü", world.tower.position.x, world.tower.position.y + 70.0f);
        });
    }
}
