#include "hud.hpp"

#include <cmath>
#include <format>
#include <optional>
#include <vector>

using namespace p5;
using namespace p5::animation;

namespace
{
    constexpr float padding = 16.0f;
    constexpr float tabHeight = 30.0f;
    constexpr float rowHeight = 62.0f;
    constexpr float enemyRowHeight = 58.0f;
    constexpr float pipHeight = 4.0f;
    constexpr float pipGap = 3.0f;
    constexpr float gap = 8.0f;
    constexpr float headerHeight = 76.0f;
    constexpr float progressHeight = 6.0f;

    constexpr std::array pages = {HudPage::attack, HudPage::utility, HudPage::enemies};

    constexpr color_t panelColor = rgba(24);
    constexpr color_t buttonColor = rgba(38);
    constexpr color_t buttonHoverColor = rgba(50);
    constexpr color_t accentColor = rgba(60, 180, 220);
    constexpr color_t goldColor = rgba(230, 200, 90);
    constexpr color_t textColor = rgba(230);
    constexpr color_t mutedTextColor = rgba(130);

    struct Rect
    {
        float x, y, w, h;
    };

    struct TabLayout
    {
        HudPage page;
        Rect rect;
    };

    struct RowLayout
    {
        const UpgradeDefinition* definition;
        Rect rect;
    };

    std::optional<UpgradeCategory> page_category(const HudPage page)
    {
        switch (page) {
            case HudPage::attack: return UpgradeCategory::attack;
            case HudPage::utility: return UpgradeCategory::utility;
            case HudPage::enemies: return std::nullopt;
        }
        return std::nullopt;
    }

    std::string_view page_name(const HudPage page)
    {
        if (const std::optional<UpgradeCategory> category = page_category(page)) {
            return upgrade_category_name(*category);
        }
        return "Gegner";
    }

    float content_top(const Hud& hud)
    {
        return hud.position.y + padding + headerHeight + tabHeight + gap * 2.0f;
    }

    bool rect_contains(const Rect& rect, const float2& point)
    {
        return point.x >= rect.x && point.x <= rect.x + rect.w && point.y >= rect.y && point.y <= rect.y + rect.h;
    }

    float2 mouse_position()
    {
        return float2 {static_cast<float>(getMouseX()), static_cast<float>(getMouseY())};
    }

    std::vector<TabLayout> layout_tabs(const Hud& hud)
    {
        const float innerWidth = hud.size.x - padding * 2.0f;
        const float tabWidth = (innerWidth - gap * static_cast<float>(pages.size() - 1)) / static_cast<float>(pages.size());
        const float y = hud.position.y + padding + headerHeight;

        std::vector<TabLayout> tabs;
        for (size_t i = 0; i < pages.size(); ++i) {
            tabs.push_back(TabLayout {
                .page = pages[i],
                .rect = Rect {hud.position.x + padding + static_cast<float>(i) * (tabWidth + gap), y, tabWidth, tabHeight},
            });
        }
        return tabs;
    }

    std::vector<RowLayout> layout_rows(const Hud& hud)
    {
        const std::optional<UpgradeCategory> category = page_category(hud.activePage);
        if (!category) {
            return {};
        }

        float y = content_top(hud);

        std::vector<RowLayout> rows;
        for (const UpgradeDefinition& definition : upgrade_definitions()) {
            if (definition.category != *category) {
                continue;
            }

            rows.push_back(RowLayout {
                .definition = &definition,
                .rect = Rect {hud.position.x + padding, y, hud.size.x - padding * 2.0f, rowHeight},
            });
            y += rowHeight + gap;
        }
        return rows;
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

        textSize(20.0f * (1.0f + 0.25f * hud.goldPunch.value()));
        textAlign(TextAlignment::topRight);
        fill(lerpColor(goldColor, rgba(255), hud.goldPunch.value() * 0.6f));
        text(std::format("{} G", static_cast<int>(std::round(hud.displayedGold))), right, y);
    }

    void show_wave_progress(const Hud& hud, const World& world)
    {
        const WaveProgress progress = wave_spawner_progress(world.waves, world.waveConfig, world.enemies.size());

        const float left = hud.position.x + padding;
        const float width = hud.size.x - padding * 2.0f;
        const float labelY = hud.position.y + padding + 32.0f;
        const float barY = labelY + 20.0f;

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

        noStroke();
        fill(buttonColor);
        rect(left, barY, width, progressHeight, BorderRadius::all(progressHeight * 0.5f));
        if (fraction > 0.0f) {
            fill(barColor);
            rect(left, barY, std::max(width * fraction, progressHeight), progressHeight, BorderRadius::all(progressHeight * 0.5f));
        }
    }

    void show_tab(const Hud& hud, const TabLayout& tab)
    {
        const bool active = tab.page == hud.activePage;
        const bool hovered = rect_contains(tab.rect, mouse_position());

        noStroke();
        fill(active ? accentColor : (hovered ? buttonHoverColor : buttonColor));
        rect(tab.rect.x, tab.rect.y, tab.rect.w, tab.rect.h, BorderRadius::all(6.0f));

        textSize(14.0f);
        textAlign(TextAlignment::center);
        fill(active ? panelColor : textColor);
        text(page_name(tab.page), tab.rect.x + tab.rect.w * 0.5f, tab.rect.y + tab.rect.h * 0.5f);
    }

    void show_row(const RowLayout& row, const World& world, const TowerStats& stats)
    {
        const UpgradeDefinition& definition = *row.definition;
        const int level = upgrade_level(world.upgrades, definition.id);
        const bool maxed = upgrade_is_maxed(definition, level);
        const int cost = upgrade_cost(definition, level);
        const bool affordable = !maxed && world.gold >= cost;
        const bool hovered = affordable && rect_contains(row.rect, mouse_position());

        fill(hovered ? buttonHoverColor : buttonColor);
        if (affordable) {
            stroke(accentColor);
            strokeWeight(2.0f);
        } else {
            noStroke();
        }
        rect(row.rect.x, row.rect.y, row.rect.w, row.rect.h, BorderRadius::all(8.0f));
        noStroke();

        const float left = row.rect.x + 12.0f;
        const float right = row.rect.x + row.rect.w - 12.0f;

        textSize(15.0f);
        textAlign(TextAlignment::topLeft);
        fill(textColor);
        text(definition.name, left, row.rect.y + 9.0f);

        UpgradeLevels nextLevels = world.upgrades;
        ++nextLevels.levels[static_cast<size_t>(definition.id)];
        const TowerStats nextStats = tower_stats_compute(world.baseStats, nextLevels);

        const std::string levelLabel = definition.maxLevel > 0 ? std::format("Lv {} / {}", level, definition.maxLevel) : std::format("Lv {}", level);
        const std::string detail = maxed
            ? std::format("{} · {}", levelLabel, definition.describe(stats))
            : std::format("{} · {} → {}", levelLabel, definition.describe(stats), definition.describe(nextStats));

        textSize(12.0f);
        textAlign(TextAlignment::topLeft);
        fill(mutedTextColor);
        text(detail, left, row.rect.y + 30.0f);

        // one segment per level, only for upgrades that have a limit
        if (definition.maxLevel > 0) {
            const float pipsWidth = row.rect.w - 24.0f - 60.0f;
            const float pipWidth = (pipsWidth - pipGap * static_cast<float>(definition.maxLevel - 1)) / static_cast<float>(definition.maxLevel);
            const float pipY = row.rect.y + row.rect.h - 12.0f;

            for (int i = 0; i < definition.maxLevel; ++i) {
                fill(i < level ? (maxed ? goldColor : accentColor) : panelColor);
                rect(left + static_cast<float>(i) * (pipWidth + pipGap), pipY, pipWidth, pipHeight, BorderRadius::all(pipHeight * 0.5f));
            }
        }

        textSize(15.0f);
        textAlign(TextAlignment::centerRight);
        fill(maxed ? mutedTextColor : (affordable ? goldColor : mutedTextColor));
        text(maxed ? std::string("MAX") : std::format("{} G", cost), right, row.rect.y + row.rect.h * 0.5f);
    }

    void show_enemies(const Hud& hud, const World& world)
    {
        const int wave = world.waves.wave;
        const std::vector<SpawnEntry>& entries = world.waveConfig.enemies;

        const float left = hud.position.x + padding;
        const float width = hud.size.x - padding * 2.0f;
        float y = content_top(hud);

        textSize(12.0f);
        textAlign(TextAlignment::topLeft);
        fill(mutedTextColor);
        text(std::format("Chance pro Spawn · Welle {}", wave), left, y);
        y += 22.0f;

        for (const SpawnEntry& entry : entries) {
            const EnemyArchetype& archetype = *entry.archetype;
            const bool unlocked = wave >= entry.weight.unlockWave;
            const float chance = spawn_table_chance(entries, entry, wave);

            noStroke();
            fill(buttonColor);
            rect(left, y, width, enemyRowHeight, BorderRadius::all(8.0f));

            constexpr float iconSize = 14.0f;
            const float iconX = left + 12.0f;
            const float iconY = y + 11.0f;
            noFill();
            stroke(unlocked ? archetype.color : mutedTextColor);
            strokeWeight(2.0f);
            rect(iconX, iconY, iconSize, iconSize);
            noStroke();

            textSize(14.0f);
            textAlign(TextAlignment::topLeft);
            fill(unlocked ? textColor : mutedTextColor);
            text(archetype.name, iconX + iconSize + 10.0f, y + 9.0f);

            textSize(11.0f);
            fill(mutedTextColor);
            text(std::format("{:.0f} Leben · {:.0f} Schaden", archetype.health, archetype.damage), iconX, y + 29.0f);
            textSize(14.0f);

            textAlign(TextAlignment::topRight);
            if (unlocked) {
                fill(textColor);
                text(std::format("{:.0f} %", chance * 100.0f), left + width - 12.0f, y + 9.0f);
            } else {
                textSize(12.0f);
                fill(mutedTextColor);
                text(std::format("ab Welle {}", entry.weight.unlockWave), left + width - 12.0f, y + 10.0f);
            }

            const float barX = left + 12.0f;
            const float barWidth = width - 24.0f;
            const float barY = y + enemyRowHeight - 11.0f;
            fill(panelColor);
            rect(barX, barY, barWidth, pipHeight, BorderRadius::all(pipHeight * 0.5f));
            if (chance > 0.0f) {
                fill(archetype.color);
                rect(barX, barY, std::max(barWidth * chance, pipHeight), pipHeight, BorderRadius::all(pipHeight * 0.5f));
            }

            y += enemyRowHeight + gap;
        }

        if (!world.waveConfig.bosses.empty() && world.waveConfig.bossWaveInterval > 0) {
            const int interval = world.waveConfig.bossWaveInterval;
            const int nextBossWave = (wave + interval - 1) / interval * interval;

            textSize(12.0f);
            textAlign(TextAlignment::topLeft);
            fill(nextBossWave == wave ? goldColor : mutedTextColor);
            text(nextBossWave == wave ? std::string("Boss-Welle!") : std::format("Nächste Boss-Welle: {}", nextBossWave), left, y + 4.0f);
        }
    }
}

Hud hud_create(const p5::Font& font, const p5::float2& position, const p5::float2& size)
{
    return Hud {
        .position = position,
        .size = size,
        .font = font,
        .activePage = HudPage::attack,
        .displayedGold = 0.0f,
        .lastGold = 0,
        .goldPunch = valueTweenJump(0.0f),
    };
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

    if (!isMouseButtonPressed(MouseButton::left)) {
        return;
    }

    const float2 mouse = mouse_position();

    for (const TabLayout& tab : layout_tabs(hud)) {
        if (rect_contains(tab.rect, mouse)) {
            hud.activePage = tab.page;
            return;
        }
    }

    for (const RowLayout& row : layout_rows(hud)) {
        if (rect_contains(row.rect, mouse)) {
            upgrade_try_buy(*row.definition, world.upgrades, world.gold);
            return;
        }
    }
}

void hud_show(const Hud& hud, const World& world)
{
    const TowerStats stats = world_tower_stats(world);

    with([&] {
        noStroke();
        fill(panelColor);
        rect(hud.position.x, hud.position.y, hud.size.x, hud.size.y);

        textFont(hud.font);
        show_header(hud, world);
        show_wave_progress(hud, world);

        for (const TabLayout& tab : layout_tabs(hud)) {
            show_tab(hud, tab);
        }
        if (hud.activePage == HudPage::enemies) {
            show_enemies(hud, world);
        } else {
            for (const RowLayout& row : layout_rows(hud)) {
                show_row(row, world, stats);
            }
        }
    });
}
