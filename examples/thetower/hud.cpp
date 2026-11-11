#include "hud.hpp"

#include <array>
#include <cmath>
#include <format>
#include <vector>

using namespace p5;
using namespace p5::animation;
using namespace ui;

namespace
{
    constexpr float padding = 16.0f;
    constexpr float gap = 8.0f;
    constexpr float headerHeight = 112.0f;
    constexpr float progressHeight = 6.0f;

    constexpr float enemyRowHeight = 40.0f;
    constexpr float enemyRowGap = 4.0f;
    constexpr float enemyTitleHeight = 22.0f;
    constexpr float bossLineHeight = 18.0f;

    constexpr std::array categories = {UpgradeCategory::attack, UpgradeCategory::defense, UpgradeCategory::utility};

    struct TabLayout
    {
        UpgradeCategory category;
        Rect rect;
    };

    struct RowLayout
    {
        const UpgradeDefinition* definition;
        Rect rect;
    };

    bool shows_boss_hint(const World& world)
    {
        return !world.waveConfig.bosses.empty() && world.waveConfig.bossWaveInterval > 0;
    }

    // the enemy overview is docked to the bottom of the panel, the upgrades scroll in the space above it
    float enemies_top(const Hud& hud, const World& world)
    {
        const float rows = static_cast<float>(world.waveConfig.enemies.size());
        const float height = padding + enemyTitleHeight + rows * (enemyRowHeight + enemyRowGap) + (shows_boss_hint(world) ? bossLineHeight : 0.0f) + padding * 0.5f;
        return hud.position.y + hud.size.y - height;
    }

    Rect upgrades_view(const Hud& hud, const World& world)
    {
        const float top = hud.position.y + padding + headerHeight + tabHeight + gap * 2.0f;
        const float bottom = enemies_top(hud, world) - gap;
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

    std::vector<RowLayout> layout_rows(const Hud& hud, const Rect& view)
    {
        float y = view.y - hud.upgradeScroll.offset;

        std::vector<RowLayout> rows;
        for (const UpgradeDefinition& definition : upgrade_definitions()) {
            if (definition.category != hud.activeCategory) {
                continue;
            }

            rows.push_back(RowLayout {
                .definition = &definition,
                .rect = Rect {view.x, y, view.w, rowHeight},
            });
            y += rowHeight + rowGap;
        }
        return rows;
    }

    float rows_height(const std::vector<RowLayout>& rows)
    {
        return rows.empty() ? 0.0f : static_cast<float>(rows.size()) * (rowHeight + rowGap) - rowGap;
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
        const WaveProgress progress = wave_spawner_progress(world.waves, world.waveConfig, world.enemies.size());

        const float left = hud.position.x + padding;
        const float width = hud.size.x - padding * 2.0f;
        const float labelY = hud.position.y + padding + 32.0f;

        std::string label;
        float fraction = 0.0f;
        color_t barColor = accentColor;

        switch (progress.phase) {
            case WavePhase::spawning:
            case WavePhase::clearing: {
                const int remaining = progress.total - progress.defeated;
                label = std::format("{} / {} besiegt · {} übrig", progress.defeated, progress.total, remaining);
                fraction = progress.total > 0 ? static_cast<float>(progress.defeated) / static_cast<float>(progress.total) : 1.0f;
                break;
            }
            case WavePhase::pause: {
                label = std::format("Nächste Welle in {:.1f}s", progress.pauseRemaining);
                fraction = 1.0f - progress.pauseRemaining / world.waveConfig.breakDuration;
                barColor = goldColor;
                break;
            }
        }

        textSize(12.0f);
        textAlign(TextAlignment::topLeft);
        fill(mutedTextColor);
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

        textSize(12.0f);
        textAlign(TextAlignment::topLeft);
        fill(mutedTextColor);
        const std::string regen = stats.healthRegen > 0.0f ? std::format(" · +{:.1f}/s", stats.healthRegen) : std::string();
        text(std::format("Leben {:.0f} / {:.0f}{}", std::ceil(health), stats.maxHealth, regen), left, labelY);

        bar(Rect {left, labelY + 20.0f, width, progressHeight}, fraction, lerpColor(dangerColor, healthColor, fraction));
    }

    void show_row(const RowLayout& row, const World& world, const TowerStats& stats)
    {
        const UpgradeDefinition& definition = *row.definition;
        const int level = upgrade_level(world.upgrades, definition.id);
        const bool maxed = upgrade_is_maxed(definition, level);
        const int price = upgrade_gold_price(definition, world.upgrades, world.startLevels);

        UpgradeLevels nextLevels = world.upgrades;
        ++nextLevels.levels[static_cast<size_t>(definition.id)];
        const TowerStats nextStats = tower_stats_compute(world.baseStats, nextLevels);

        const std::string levelLabel = definition.maxLevel > 0 ? std::format("Lv {} / {}", level, definition.maxLevel) : std::format("Lv {}", level);

        upgrade_row(row.rect, UpgradeRow {
            .name = definition.name,
            .detail = maxed
                ? std::format("{} · {}", levelLabel, definition.describe(stats))
                : std::format("{} · {} → {}", levelLabel, definition.describe(stats), definition.describe(nextStats)),
            .level = level,
            .maxLevel = definition.maxLevel,
            .price = maxed ? std::string("MAX") : std::format("{} G", price),
            .priceColor = goldColor,
            .affordable = !maxed && world.gold >= price,
            .maxed = maxed,
        });
    }

    void show_enemies(const Hud& hud, const World& world)
    {
        const int wave = world.waves.wave;
        const std::vector<SpawnEntry>& entries = world.waveConfig.enemies;

        const float left = hud.position.x + padding;
        const float width = hud.size.x - padding * 2.0f;
        const float top = enemies_top(hud, world);

        noStroke();
        fill(buttonColor);
        rect(hud.position.x, top, hud.size.x, 1.0f);

        float y = top + padding;

        textSize(12.0f);
        textAlign(TextAlignment::topLeft);
        fill(mutedTextColor);
        text(std::format("Gegner · Welle {}", wave), left, y);
        y += enemyTitleHeight;

        for (const SpawnEntry& entry : entries) {
            const EnemyArchetype& archetype = *entry.archetype;
            const bool unlocked = wave >= entry.weight.unlockWave;
            const float chance = spawn_table_chance(entries, entry, wave);

            constexpr float iconSize = 10.0f;
            noFill();
            stroke(unlocked ? archetype.color : mutedTextColor);
            strokeWeight(2.0f);
            rect(left, y + 3.0f, iconSize, iconSize);
            noStroke();

            textSize(13.0f);
            textAlign(TextAlignment::topLeft);
            fill(unlocked ? textColor : mutedTextColor);
            text(archetype.name, left + iconSize + 8.0f, y);

            textSize(11.0f);
            fill(mutedTextColor);
            text(std::format("{:.0f} Leben · {:.0f} Schaden", archetype.health, archetype.damage), left + iconSize + 8.0f, y + 17.0f);

            textAlign(TextAlignment::topRight);
            if (unlocked) {
                textSize(13.0f);
                fill(textColor);
                text(std::format("{:.0f} %", chance * 100.0f), left + width, y);
            } else {
                fill(mutedTextColor);
                text(std::format("ab Welle {}", entry.weight.unlockWave), left + width, y + 1.0f);
            }

            bar(Rect {left, y + enemyRowHeight - 6.0f, width, 3.0f}, chance, archetype.color);

            y += enemyRowHeight + enemyRowGap;
        }

        if (shows_boss_hint(world)) {
            const int interval = world.waveConfig.bossWaveInterval;
            const int nextBossWave = (wave + interval - 1) / interval * interval;

            textSize(12.0f);
            textAlign(TextAlignment::topLeft);
            fill(nextBossWave == wave ? goldColor : mutedTextColor);
            text(nextBossWave == wave ? std::string("Boss-Welle!") : std::format("Nächste Boss-Welle: {}", nextBossWave), left, y);
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

    const Rect view = upgrades_view(hud, world);
    scroll_list_update(hud.upgradeScroll, view, rows_height(layout_rows(hud, view)));

    for (const TabLayout& tab : layout_tabs(hud)) {
        if (clicked(tab.rect)) {
            hud.activeCategory = tab.category;
            hud.upgradeScroll = {};
            return;
        }
    }

    // rows scrolled out of the view must not be clickable
    if (!clicked(view)) {
        return;
    }
    for (const RowLayout& row : layout_rows(hud, view)) {
        if (clicked(row.rect)) {
            upgrade_buy_with_gold(*row.definition, world.upgrades, world.startLevels, world.gold);
            return;
        }
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

        const Rect view = upgrades_view(hud, world);
        const std::vector<RowLayout> rows = layout_rows(hud, view);

        withClip(view.x - 2.0f, view.y, view.w + 4.0f, view.h, [&] {
            for (const RowLayout& row : rows) {
                show_row(row, world, stats);
            }
        });
        scroll_list_show_bar(hud.upgradeScroll, view, rows_height(rows));

        show_enemies(hud, world);
    });
}
