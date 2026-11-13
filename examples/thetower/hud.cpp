#include "hud.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <utility>
#include <vector>

#include "upgrade_list.hpp"

using namespace p5;
using namespace p5::animation;
using namespace ui;

namespace
{
    constexpr float padding = 16.0f;
    constexpr float gap = 8.0f;
    constexpr float progressHeight = 8.0f;

    constexpr float titleHeight = 44.0f;
    constexpr float statusLabelHeight = 18.0f;
    constexpr float statusRowGap = 12.0f;
    constexpr float statusRowHeight = statusLabelHeight + progressHeight;
    constexpr float headerHeight = titleHeight + statusRowHeight * 2.0f + statusRowGap + padding;

    constexpr float balancePadding = 11.0f;
    constexpr float balanceGap = 6.0f;

    constexpr float enemyStripPadding = 12.0f;
    constexpr float enemyTitleHeight = 22.0f;
    constexpr float enemyChipHeight = 24.0f;
    constexpr float enemyBarGap = 8.0f;
    constexpr float enemyBarHeight = 8.0f;
    constexpr float enemyStripHeight = enemyStripPadding * 2.0f + enemyTitleHeight + enemyChipHeight + enemyBarGap + enemyBarHeight;

    // the enemy strip is docked to the bottom of the panel, the upgrades scroll in the space above it
    struct Layout
    {
        rect2f header;
        rect2f divider;
        rect2f tabs;
        rect2f upgrades;
        rect2f enemies;
    };

    Layout layout(const Hud& hud)
    {
        rect2f panel {hud.position.x, hud.position.y, hud.size.x, hud.size.y};

        Layout result = {};
        result.enemies = cutBottom(panel, enemyStripHeight);
        cutBottom(panel, gap);

        panel = inset(panel, padding, 0.0f);
        cutTop(panel, padding);
        result.header = cutTop(panel, headerHeight);
        result.divider = cutTop(panel, 1.0f);
        cutTop(panel, padding);
        result.tabs = cutTop(panel, tabHeight);
        cutTop(panel, gap * 2.0f);
        result.upgrades = panel;
        return result;
    }

    bool shows_boss_hint(const World& world)
    {
        return !world.waveConfig.bosses.empty() && world.waveConfig.bossWaveInterval > 0;
    }

    UpgradeListLayout layout_upgrades(const Hud& hud, const rect2f& view)
    {
        return upgrade_list_layout(hud.activeCategory, view, hud.upgradeScroll.offset);
    }

    // a balance in its own colour, laid out from the right edge inwards and returning the space it took
    float show_balance(const rect2f& row, const std::string& label, const color_t color, const float size, const float emphasis)
    {
        font_bold();
        textSize(size);

        const float height = size + 11.0f;
        const float width = textWidth(label) + balancePadding * 2.0f;
        const rect2f chipRect {row.left + row.width - width, center(row).y - height * 0.5f, width, height};
        chip(chipRect, color, emphasis);

        textAlign(TextAlignment::center);
        fill(lerpColor(color, rgba(255), emphasis * 0.5f));
        text(label, center(chipRect).x, center(chipRect).y);
        return chipRect.width;
    }

    void show_title(rect2f row, const Hud& hud, const World& world)
    {
        const float punch = hud.goldPunch.value();

        const float gold = show_balance(row, std::format("{} G", static_cast<int>(std::round(hud.displayedGold))), goldColor, 16.0f + 2.0f * punch, punch);
        cutRight(row, gold + balanceGap);
        show_balance(row, std::format("{} C", world.coins), coinColor, 14.0f, 0.0f);

        font_medium();
        textSize(11.0f);
        textLetterSpacing(1.4f);
        textAlign(TextAlignment::topLeft);
        fill(mutedTextColor);
        text("WELLE", row.left, row.top);
        textLetterSpacing(0.0f);

        font_bold();
        textSize(24.0f);
        fill(textColor);
        text(std::format("{}", world.waves.wave), row.left, row.top + 13.0f);
    }

    // a label with a bar underneath it
    void show_status(rect2f row, const std::string_view label, const std::string_view value, const float fraction, const color_t barColor)
    {
        const rect2f labelRow = cutTop(row, statusLabelHeight);

        font_medium();
        textSize(12.0f);
        textAlign(TextAlignment::centerLeft);
        fill(mutedTextColor);
        text(label, labelRow.left, center(labelRow).y);

        font_bold();
        textAlign(TextAlignment::centerRight);
        fill(secondaryTextColor);
        text(value, labelRow.left + labelRow.width, center(labelRow).y);

        bar(cutTop(row, progressHeight), fraction, barColor);
    }

    void show_wave_progress(const rect2f& row, const World& world)
    {
        const WaveProgress progress = wave_spawner_progress(world.waves, world.waveConfig);

        show_status(row, std::format("Nächste Welle in {:.0f}s", std::ceil(progress.timeRemaining)), std::format("{} aktiv", world.enemies.size()), 1.0f - progress.timeRemaining / progress.duration, accentColor);
    }

    void show_health(const rect2f& row, const World& world, const TowerStats& stats)
    {
        const float health = tower_health(world.tower, stats.maxHealth);
        const float fraction = stats.maxHealth > 0.0f ? health / stats.maxHealth : 0.0f;

        const std::string label = stats.healthRegen > 0.0f ? std::format("Leben · +{:.1f}/s", stats.healthRegen) : std::string("Leben");

        show_status(row, label, std::format("{:.0f} / {:.0f}", std::ceil(health), stats.maxHealth), fraction, lerpColor(dangerColor, healthColor, fraction));
    }

    UpgradeLine describe_line(const UpgradeDefinition& definition, const World& world)
    {
        const int price = upgrade_gold_price(definition, world.upgrades, world.startLevels);
        const UpgradeCurrency gold = {.symbol = "G", .color = goldColor, .balance = world.gold};
        return upgrade_line_describe(definition, world.baseStats, world.upgrades, "Lv", price, gold);
    }

    // the icon of an enemy type, drawn the way the type is drawn on the playfield
    void show_enemy_icon(const EnemyArchetype& archetype, const float2& position, const float size, const bool unlocked)
    {
        const color_t color = unlocked ? archetype.color : mutedTextColor;

        if (unlocked) {
            glow(rect2f {position.x - size * 0.5f, position.y - size * 0.5f, size, size}, color, 0.8f, size * 0.5f, 5.0f);
        }

        fill(lerpColor(rgba(20, 22, 28), color, 0.28f));
        stroke(color);
        strokeWeight(2.0f);
        enemy_shape_draw(archetype.shape, position, size);
        noStroke();
    }

    // the mix of the current wave as one stacked bar, rounded on its outer ends like every other bar
    void show_enemy_mix(const rect2f& barRect, const std::vector<SpawnEntry>& entries, const int wave)
    {
        const float radius = barRect.height * 0.5f;

        noStroke();
        fill(surfaceColor);
        rect(barRect.left, barRect.top, barRect.width, barRect.height, BorderRadius::all(radius));

        std::vector<std::pair<rect2f, color_t>> segments;
        rect2f remaining = barRect;
        for (const SpawnEntry& entry : entries) {
            const rect2f segment = cutLeft(remaining, barRect.width * spawn_table_chance(entries, entry, wave));
            if (segment.width > 1.0f) {
                segments.emplace_back(segment, entry.archetype->color);
            }
        }

        for (size_t i = 0; i < segments.size(); ++i) {
            const auto& [segment, color] = segments[i];
            const bool first = i == 0;
            const bool last = i + 1 == segments.size();
            const BorderRadius corners {
                .topLeft = CornerRadius::circular(first ? radius : 0.0f),
                .topRight = CornerRadius::circular(last ? radius : 0.0f),
                .bottomRight = CornerRadius::circular(last ? radius : 0.0f),
                .bottomLeft = CornerRadius::circular(first ? radius : 0.0f),
            };

            fill(color);
            rect(segment.left, segment.top, last ? segment.width : segment.width - 2.0f, segment.height, corners);
        }
    }

    // details of the hovered type float above the strip
    void show_enemy_tooltip(const rect2f& strip, const rect2f& inner, const EnemyArchetype& archetype)
    {
        const std::string details = std::format("{} · {:.0f} Leben · {:.0f} Schaden", archetype.name, archetype.health, archetype.damage);

        font_medium();
        textSize(12.0f);
        const float tipWidth = textWidth(details) + 22.0f;
        const rect2f tip {std::clamp(mouse().x - tipWidth * 0.5f, inner.left, inner.left + inner.width - tipWidth), strip.top - 36.0f, tipWidth, 28.0f};

        surface(tip, buttonColor, withOpacity(archetype.color, 0.5f), 8.0f);
        textAlign(TextAlignment::center);
        fill(textColor);
        text(details, center(tip).x, center(tip).y);
    }

    void show_enemies(const rect2f& strip, const World& world)
    {
        const int wave = world.waves.wave;
        const std::vector<SpawnEntry>& entries = world.waveConfig.enemies;
        const int count = static_cast<int>(entries.size());

        noStroke();
        fill(withOpacity(accentColor, 0.18f));
        rect(strip.left, strip.top, strip.width, 1.0f);

        const rect2f inner = inset(strip, padding, enemyStripPadding);
        rect2f content = inner;

        const rect2f titleRow = cutTop(content, enemyTitleHeight);
        const rect2f chipRow = cutTop(content, enemyChipHeight);
        cutTop(content, enemyBarGap);
        const rect2f barRect = cutTop(content, enemyBarHeight);

        font_medium();
        textSize(11.0f);
        textLetterSpacing(1.2f);
        textAlign(TextAlignment::centerLeft);
        fill(mutedTextColor);
        text("GEGNER", titleRow.left, center(titleRow).y);
        textLetterSpacing(0.0f);

        if (shows_boss_hint(world)) {
            const int interval = world.waveConfig.bossWaveInterval;
            const int nextBossWave = (wave + interval - 1) / interval * interval;
            const bool now = nextBossWave == wave;

            if (now) {
                show_balance(titleRow, "BOSS-WELLE", dangerColor, 12.0f, 1.0f);
            } else {
                font_medium();
                textSize(12.0f);
                textAlign(TextAlignment::centerRight);
                fill(mutedTextColor);
                text(std::format("Boss: Welle {}", nextBossWave), titleRow.left + titleRow.width, center(titleRow).y);
            }
        }

        // one chip per enemy type: icon and spawn chance
        for (int i = 0; i < count; ++i) {
            const SpawnEntry& entry = entries[static_cast<size_t>(i)];
            const EnemyArchetype& archetype = *entry.archetype;
            const bool unlocked = wave >= entry.weight.unlockWave;
            const rect2f chip = column(chipRow, i, count);
            const float centerY = center(chip).y;

            constexpr float iconSize = 13.0f;
            show_enemy_icon(archetype, float2 {chip.left + 1.0f + iconSize * 0.5f, centerY}, iconSize, unlocked);

            textAlign(TextAlignment::centerLeft);
            if (unlocked) {
                font_bold();
                textSize(14.0f);
                fill(textColor);
                text(std::format("{:.0f}%", spawn_table_chance(entries, entry, wave) * 100.0f), chip.left + iconSize + 9.0f, centerY);
            } else {
                font_medium();
                textSize(12.0f);
                fill(mutedTextColor);
                text(std::format("ab W{}", entry.weight.unlockWave), chip.left + iconSize + 9.0f, centerY);
            }
        }

        show_enemy_mix(barRect, entries, wave);

        for (int i = 0; i < count; ++i) {
            if (contains(column(chipRow, i, count), mouse())) {
                show_enemy_tooltip(strip, inner, *entries[static_cast<size_t>(i)].archetype);
            }
        }
    }
}

Hud hud_create(const ui::Fonts& fonts)
{
    return Hud {
        .position = {},
        .size = {},
        .fonts = fonts,
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

    // approach the real value, then snap so the last fraction does not crawl
    hud.displayedGold = damp(hud.displayedGold, static_cast<float>(world.gold), 12.0f, deltaTime);
    if (std::abs(static_cast<float>(world.gold) - hud.displayedGold) < 0.5f) {
        hud.displayedGold = static_cast<float>(world.gold);
    }

    const Layout l = layout(hud);
    scroll_list_update(hud.upgradeScroll, l.upgrades, layout_upgrades(hud, l.upgrades).contentHeight);

    if (const std::optional<UpgradeCategory> category = upgrade_tabs_clicked(upgrade_tabs_layout(l.tabs))) {
        hud.activeCategory = *category;
        hud.upgradeScroll = {};
        return;
    }

    if (const UpgradeDefinition* definition = upgrade_list_clicked(layout_upgrades(hud, l.upgrades), l.upgrades)) {
        world_buy_upgrade(world, *definition);
    }
}

void hud_show(const Hud& hud, const World& world)
{
    const TowerStats stats = world_tower_stats(world);
    const Layout l = layout(hud);

    withClip(hud.position.x, hud.position.y, hud.size.x, hud.size.y, [&] {
        noStroke();
        fill(panelColor);
        rect(hud.position.x, hud.position.y, hud.size.x, hud.size.y);

        // the panel is separated from the playfield by a lit edge instead of a hard cut
        glow(rect2f {hud.position.x - 2.0f, hud.position.y, 2.0f, hud.size.y}, accentColor, 0.5f, 1.0f, 14.0f);
        fill(withOpacity(accentColor, 0.22f));
        rect(hud.position.x, hud.position.y, 1.0f, hud.size.y);

        set_fonts(hud.fonts);

        rect2f header = l.header;
        show_title(cutTop(header, titleHeight), hud, world);
        show_wave_progress(cutTop(header, statusRowHeight), world);
        cutTop(header, statusRowGap);
        show_health(cutTop(header, statusRowHeight), world, stats);
        hairline(l.divider);

        upgrade_tabs_show(upgrade_tabs_layout(l.tabs), hud.activeCategory);
        upgrade_list_show(layout_upgrades(hud, l.upgrades), l.upgrades, hud.upgradeScroll, [&](const UpgradeDefinition& definition) {
            return describe_line(definition, world);
        });

        show_enemies(l.enemies, world);
    });
}
