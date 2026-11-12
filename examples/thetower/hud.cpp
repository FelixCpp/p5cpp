#include "hud.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <vector>

#include "upgrade_list.hpp"

using namespace p5;
using namespace p5::animation;
using namespace ui;

namespace
{
    constexpr float padding = 16.0f;
    constexpr float gap = 8.0f;
    constexpr float headerHeight = 112.0f;
    constexpr float progressHeight = 6.0f;

    constexpr float enemyStripPadding = 12.0f;
    constexpr float enemyTitleHeight = 22.0f;
    constexpr float enemyChipHeight = 22.0f;
    constexpr float enemyBarHeight = 6.0f;
    constexpr float enemyStripHeight = enemyStripPadding + enemyTitleHeight + enemyChipHeight + 6.0f + enemyBarHeight + enemyStripPadding;

    constexpr std::array categories = {UpgradeCategory::attack, UpgradeCategory::defense, UpgradeCategory::utility};

    struct TabLayout
    {
        UpgradeCategory category;
        Rect rect;
    };

    bool shows_boss_hint(const World& world)
    {
        return !world.waveConfig.bosses.empty() && world.waveConfig.bossWaveInterval > 0;
    }

    // the enemy strip is docked to the bottom of the panel, the upgrades scroll in the space above it
    float enemies_top(const Hud& hud)
    {
        return hud.position.y + hud.size.y - enemyStripHeight;
    }

    Rect upgrades_view(const Hud& hud)
    {
        const float top = hud.position.y + padding + headerHeight + tabHeight + gap * 2.0f;
        const float bottom = enemies_top(hud) - gap;
        return Rect {hud.position.x + padding, top, hud.size.x - padding * 2.0f, std::max(bottom - top, 0.0f)};
    }

    std::vector<TabLayout> layout_tabs(const Hud& hud)
    {
        const float innerWidth = hud.size.x - padding * 2.0f;
        const float tabWidth = (innerWidth - gap * static_cast<float>(categories.size() - 1)) / static_cast<float>(categories.size());
        const float y = hud.position.y + padding + headerHeight;

        std::vector<TabLayout> tabs;
        for (size_t i = 0; i < categories.size(); ++i) {
            tabs.push_back(TabLayout {
                .category = categories[i],
                .rect = Rect {hud.position.x + padding + static_cast<float>(i) * (tabWidth + gap), y, tabWidth, tabHeight},
            });
        }
        return tabs;
    }

    UpgradeListLayout layout_upgrades(const Hud& hud)
    {
        return upgrade_list_layout(hud.activeCategory, upgrades_view(hud), hud.upgradeScroll.offset);
    }

    void show_header(const Hud& hud, const World& world)
    {
        const float y = hud.position.y + padding;
        const float left = hud.position.x + padding;
        const float right = hud.position.x + hud.size.x - padding;

        textSize(20.0f);
        textAlign(TextAlignment::topLeft);
        fill(textColor);
        text(std::format("Welle {}", world.waves.wave), left, y);

        const std::string gold = std::format("{} G", static_cast<int>(std::round(hud.displayedGold)));
        const float goldWidth = textWidth(gold);

        textSize(20.0f * (1.0f + 0.25f * hud.goldPunch.value()));
        textAlign(TextAlignment::topRight);
        fill(lerpColor(goldColor, rgba(255), hud.goldPunch.value() * 0.6f));
        text(gold, right, y);

        textSize(16.0f);
        fill(coinColor);
        text(std::format("{} C", world.coins), right - goldWidth - 14.0f, y + 3.0f);
    }

    void show_wave_progress(const Hud& hud, const World& world)
    {
        const WaveProgress progress = wave_spawner_progress(world.waves, world.waveConfig);

        const float left = hud.position.x + padding;
        const float width = hud.size.x - padding * 2.0f;
        const float labelY = hud.position.y + padding + 32.0f;

        const std::string label = std::format("Nächste Welle in {:.0f}s · {} Gegner aktiv", std::ceil(progress.timeRemaining), world.enemies.size());
        const float fraction = 1.0f - progress.timeRemaining / progress.duration;
        const color_t barColor = accentColor;

        textSize(13.0f);
        textAlign(TextAlignment::topLeft);
        fill(secondaryTextColor);
        text(label, left, labelY);

        bar(Rect {left, labelY + 20.0f, width, progressHeight}, fraction, barColor);
    }

    void show_health(const Hud& hud, const World& world, const TowerStats& stats)
    {
        const float left = hud.position.x + padding;
        const float width = hud.size.x - padding * 2.0f;
        const float labelY = hud.position.y + padding + 70.0f;

        const float health = tower_health(world.tower, stats.maxHealth);
        const float fraction = stats.maxHealth > 0.0f ? health / stats.maxHealth : 0.0f;

        textSize(13.0f);
        textAlign(TextAlignment::topLeft);
        fill(secondaryTextColor);
        const std::string regen = stats.healthRegen > 0.0f ? std::format(" · +{:.1f}/s", stats.healthRegen) : std::string();
        text(std::format("Leben {:.0f} / {:.0f}{}", std::ceil(health), stats.maxHealth, regen), left, labelY);

        bar(Rect {left, labelY + 20.0f, width, progressHeight}, fraction, lerpColor(dangerColor, healthColor, fraction));
    }

    UpgradeLine describe_line(const UpgradeDefinition& definition, const World& world, const TowerStats& stats)
    {
        const int level = upgrade_level(world.upgrades, definition.id);
        const bool maxed = upgrade_is_maxed(definition, level);
        const int price = upgrade_gold_price(definition, world.upgrades, world.startLevels);

        UpgradeLevels nextLevels = world.upgrades;
        ++nextLevels.levels[static_cast<size_t>(definition.id)];
        const TowerStats nextStats = tower_stats_compute(world.baseStats, nextLevels);

        return UpgradeLine {
            .name = definition.name,
            .levelLabel = std::format("Lv {}", level),
            .value = definition.describe(stats),
            .nextValue = maxed ? std::string() : definition.describe(nextStats),
            .level = level,
            .maxLevel = definition.maxLevel,
            .price = maxed ? std::string("MAX") : std::format("{} G", price),
            .priceColor = goldColor,
            .affordable = !maxed && world.gold >= price,
            .maxed = maxed,
        };
    }

    Rect enemy_chip(const Hud& hud, const size_t index, const size_t count)
    {
        const float width = (hud.size.x - padding * 2.0f) / static_cast<float>(count);
        const float y = enemies_top(hud) + enemyStripPadding + enemyTitleHeight;
        return Rect {hud.position.x + padding + static_cast<float>(index) * width, y, width, enemyChipHeight};
    }

    void show_enemies(const Hud& hud, const World& world)
    {
        const int wave = world.waves.wave;
        const std::vector<SpawnEntry>& entries = world.waveConfig.enemies;

        const float left = hud.position.x + padding;
        const float right = hud.position.x + hud.size.x - padding;
        const float width = right - left;
        const float top = enemies_top(hud);

        noStroke();
        fill(buttonColor);
        rect(hud.position.x, top, hud.size.x, 1.0f);

        const float titleY = top + enemyStripPadding;
        textSize(13.0f);
        textAlign(TextAlignment::topLeft);
        fill(secondaryTextColor);
        text(std::format("Gegner · Welle {}", wave), left, titleY);

        if (shows_boss_hint(world)) {
            const int interval = world.waveConfig.bossWaveInterval;
            const int nextBossWave = (wave + interval - 1) / interval * interval;

            textAlign(TextAlignment::topRight);
            fill(nextBossWave == wave ? goldColor : mutedTextColor);
            text(nextBossWave == wave ? std::string("Boss-Welle!") : std::format("Boss: Welle {}", nextBossWave), right, titleY);
        }

        // one chip per enemy type: icon and spawn chance
        for (size_t i = 0; i < entries.size(); ++i) {
            const SpawnEntry& entry = entries[i];
            const EnemyArchetype& archetype = *entry.archetype;
            const bool unlocked = wave >= entry.weight.unlockWave;
            const Rect chip = enemy_chip(hud, i, entries.size());
            const float centerY = chip.y + chip.h * 0.5f;

            constexpr float iconSize = 12.0f;
            noFill();
            stroke(unlocked ? archetype.color : mutedTextColor);
            strokeWeight(2.0f);
            rect(chip.x + 1.0f, centerY - iconSize * 0.5f, iconSize, iconSize);
            noStroke();

            textAlign(TextAlignment::centerLeft);
            if (unlocked) {
                textSize(15.0f);
                fill(textColor);
                text(std::format("{:.0f} %", spawn_table_chance(entries, entry, wave) * 100.0f), chip.x + iconSize + 8.0f, centerY);
            } else {
                textSize(13.0f);
                fill(mutedTextColor);
                text(std::format("ab W{}", entry.weight.unlockWave), chip.x + iconSize + 8.0f, centerY);
            }
        }

        // the mix of the current wave as one stacked bar
        const Rect barRect = {left, top + enemyStripPadding + enemyTitleHeight + enemyChipHeight + 6.0f, width, enemyBarHeight};
        bar(barRect, 0.0f, buttonColor);
        float x = barRect.x;
        for (const SpawnEntry& entry : entries) {
            const float segment = barRect.w * spawn_table_chance(entries, entry, wave);
            if (segment > 0.0f) {
                fill(entry.archetype->color);
                rect(x, barRect.y, segment, barRect.h);
                x += segment;
            }
        }

        // details of the hovered type float above the strip
        for (size_t i = 0; i < entries.size(); ++i) {
            if (!contains(enemy_chip(hud, i, entries.size()), mouse())) {
                continue;
            }

            const EnemyArchetype& archetype = *entries[i].archetype;
            const std::string details = std::format("{} · {:.0f} Leben · {:.0f} Schaden", archetype.name, archetype.health, archetype.damage);

            textSize(13.0f);
            const float tipWidth = textWidth(details) + 20.0f;
            const float tipX = std::clamp(mouse().x - tipWidth * 0.5f, left, right - tipWidth);
            const float tipY = top - 34.0f;

            fill(buttonHoverColor);
            rect(tipX, tipY, tipWidth, 28.0f, BorderRadius::all(6.0f));
            textAlign(TextAlignment::center);
            fill(textColor);
            text(details, tipX + tipWidth * 0.5f, tipY + 14.0f);
        }
    }
}

Hud hud_create(const p5::Font& font)
{
    return Hud {
        .position = {},
        .size = {},
        .font = font,
        .activeCategory = UpgradeCategory::attack,
        .upgradeScroll = {},
        .displayedGold = 0.0f,
        .lastGold = 0,
        .goldPunch = valueTweenJump(0.0f),
    };
}

void hud_reset(Hud& hud)
{
    hud.upgradeScroll = {};
    hud.displayedGold = 0.0f;
    hud.lastGold = 0;
    hud.goldPunch = valueTweenJump(0.0f);
}

void hud_update(Hud& hud, World& world, const float deltaTime)
{
    if (world.gold > hud.lastGold) {
        hud.goldPunch = valueTween(1.0f, 0.0f, 0.35f, curves::easeOutCubic);
    }
    hud.lastGold = world.gold;
    hud.goldPunch.advance(deltaTime);

    // exponential approach, then snap so the last fraction does not crawl
    hud.displayedGold += (static_cast<float>(world.gold) - hud.displayedGold) * (1.0f - std::exp(-12.0f * deltaTime));
    if (std::abs(static_cast<float>(world.gold) - hud.displayedGold) < 0.5f) {
        hud.displayedGold = static_cast<float>(world.gold);
    }

    const Rect view = upgrades_view(hud);
    scroll_list_update(hud.upgradeScroll, view, layout_upgrades(hud).contentHeight);

    for (const TabLayout& tab : layout_tabs(hud)) {
        if (clicked(tab.rect)) {
            hud.activeCategory = tab.category;
            hud.upgradeScroll = {};
            return;
        }
    }

    if (const UpgradeDefinition* definition = upgrade_list_clicked(layout_upgrades(hud), view)) {
        upgrade_buy_with_gold(*definition, world.upgrades, world.startLevels, world.gold);
    }
}

void hud_show(const Hud& hud, const World& world)
{
    const TowerStats stats = world_tower_stats(world);

    withClip(hud.position.x, hud.position.y, hud.size.x, hud.size.y, [&] {
        noStroke();
        fill(panelColor);
        rect(hud.position.x, hud.position.y, hud.size.x, hud.size.y);

        textFont(hud.font);
        show_header(hud, world);
        show_wave_progress(hud, world);
        show_health(hud, world, stats);

        for (const TabLayout& tab : layout_tabs(hud)) {
            ui::tab(tab.rect, upgrade_category_name(tab.category), tab.category == hud.activeCategory);
        }

        upgrade_list_show(layout_upgrades(hud), upgrades_view(hud), hud.upgradeScroll, [&](const UpgradeDefinition& definition) {
            return describe_line(definition, world, stats);
        });

        show_enemies(hud, world);
    });
}
