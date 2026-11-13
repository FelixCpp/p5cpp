#include "bestiary.hpp"

#include <algorithm>
#include <format>

#include "enemy_roster.hpp"
#include "enemy_shape.hpp"

using namespace p5;
using namespace ui;

namespace
{
    constexpr float padding = 20.0f;
    constexpr float maxPanelWidth = 560.0f;
    constexpr float headerHeight = 54.0f;
    constexpr float closeHeight = 44.0f;
    constexpr float rowHeight = 76.0f;
    constexpr float rowGap = 6.0f;
    constexpr float iconColumnWidth = 40.0f;
    constexpr float stateWidth = 88.0f;

    float content_height()
    {
        const float rows = static_cast<float>(enemy_roster().size());
        return rows * rowHeight + std::max(rows - 1.0f, 0.0f) * rowGap;
    }

    void show_icon(const EnemyArchetype& archetype, const float2& position, const float size, const bool unlocked)
    {
        const color_t color = unlocked ? archetype.color : mutedTextColor;

        if (unlocked) {
            glow(rect2f {position.x - size * 0.5f, position.y - size * 0.5f, size, size}, color, 1.0f, size * 0.5f, 8.0f);
        }

        fill(lerpColor(rgba(20, 22, 28), color, 0.28f));
        stroke(color);
        strokeWeight(2.0f);
        enemy_shape_draw(archetype.shape, position, size);
        noStroke();
    }

    // either how much of the running wave this type makes up, or the wave it first shows up in
    void show_state(const rect2f& area, const SpawnEntry& entry, const int wave, const bool unlocked)
    {
        const rect2f chipRect {area.left + area.width - stateWidth, center(area).y - 14.0f, stateWidth, 28.0f};

        if (!unlocked) {
            surface(chipRect, panelColor, withOpacity(borderColor, 0.25f), 10.0f);
            font_medium();
            textSize(12.0f);
            textAlign(TextAlignment::center);
            fill(mutedTextColor);
            text(std::format("ab Welle {}", entry.weight.unlockWave), center(chipRect).x, center(chipRect).y);
            return;
        }

        if (wave <= 0) {
            font_medium();
            textSize(12.0f);
            textAlign(TextAlignment::centerRight);
            fill(mutedTextColor);
            text(std::format("ab Welle {}", entry.weight.unlockWave), chipRect.left + chipRect.width, center(chipRect).y);
            return;
        }

        const float share = spawn_table_chance(enemy_roster(), entry, wave);

        chip(chipRect, entry.archetype->color, share);
        font_bold();
        textSize(14.0f);
        textAlign(TextAlignment::center);
        fill(entry.archetype->color);
        text(std::format("{:.0f} %", share * 100.0f), center(chipRect).x, center(chipRect).y);
    }

    void show_row(const rect2f& row, const SpawnEntry& entry, const int wave)
    {
        const EnemyArchetype& archetype = *entry.archetype;
        const bool unlocked = wave <= 0 || wave >= entry.weight.unlockWave;

        surface(row, surfaceColor, withOpacity(unlocked ? archetype.color : borderColor, unlocked ? 0.28f : 0.18f), 10.0f);

        rect2f content = inset(row, 14.0f, 0.0f);
        const rect2f iconColumn = cutLeft(content, iconColumnWidth);
        show_icon(archetype, float2 {center(iconColumn).x - 6.0f, center(row).y}, 24.0f, unlocked);

        show_state(content, entry, wave, unlocked);
        cutRight(content, stateWidth + 10.0f);

        cutTop(content, 12.0f);
        const rect2f nameRow = cutTop(content, 19.0f);
        const rect2f traitRow = cutTop(content, 17.0f);
        const rect2f statsRow = cutTop(content, 17.0f);

        font_bold();
        textSize(16.0f);
        textAlign(TextAlignment::centerLeft);
        fill(unlocked ? textColor : secondaryTextColor);
        text(archetype.name, nameRow.left, center(nameRow).y);

        font_medium();
        textSize(12.0f);
        fill(unlocked ? lerpColor(archetype.color, textColor, 0.25f) : mutedTextColor);
        text(archetype.trait.empty() ? "Läuft auf den Turm zu und schlägt zu" : archetype.trait, traitRow.left, center(traitRow).y);

        std::string stats = std::format("{:.0f} Leben", archetype.health);
        if (archetype.shield > 0.0f) {
            stats += std::format(" · {:.0f} Schild", archetype.shield);
        }
        stats += std::format(" · {:.0f} Schaden · {:.1f} m/s", archetype.damage, archetype.speed / 10.0f);

        textSize(12.0f);
        fill(mutedTextColor);
        text(stats, statsRow.left, center(statsRow).y);
    }
}

BestiaryLayout bestiary_layout(const float width, const float height)
{
    const float panelWidth = std::min(maxPanelWidth, std::max(width - padding * 2.0f, 1.0f));
    const float wanted = headerHeight + content_height() + closeHeight + padding * 3.0f;
    const float panelHeight = std::min(wanted, std::max(height - padding * 2.0f, 1.0f));

    rect2f panel {(width - panelWidth) * 0.5f, (height - panelHeight) * 0.5f, panelWidth, panelHeight};

    rect2f area = inset(panel, padding, 0.0f);
    cutTop(area, padding);
    cutTop(area, headerHeight);
    rect2f footer = cutBottom(area, closeHeight);
    const rect2f close = inset(footer, std::max((footer.width - 220.0f) * 0.5f, 0.0f), 0.0f);
    cutBottom(area, padding);

    return BestiaryLayout {
        .panel = panel,
        .list = area,
        .close = close,
        .contentHeight = content_height(),
    };
}

bool bestiary_update(Bestiary& bestiary, const BestiaryLayout& layout)
{
    scroll_list_update(bestiary.scroll, layout.list, layout.contentHeight);

    if (isKeyPressed(Key::Escape) || clicked(layout.close)) {
        return true;
    }
    // a click anywhere outside the panel closes it as well
    return isMouseButtonPressed(MouseButton::left) && !contains(layout.panel, mouse());
}

void bestiary_show(const Bestiary& bestiary, const BestiaryLayout& layout, const float width, const float height, const int wave)
{
    with([&] {
        noStroke();
        fill(rgba(0, 0, 0, 180));
        rect(0.0f, 0.0f, width, height);

        glow(layout.panel, accentColor, 0.7f, 18.0f, 24.0f);
        surface(layout.panel, panelColor, withOpacity(borderColor, 0.45f), 18.0f);

        const rect2f header {layout.panel.left + padding, layout.panel.top + padding, layout.panel.width - padding * 2.0f, headerHeight};

        font_medium();
        textSize(11.0f);
        textLetterSpacing(2.0f);
        textAlign(TextAlignment::topLeft);
        fill(mutedTextColor);
        text("GEGNER", header.left, header.top + 2.0f);
        textLetterSpacing(0.0f);

        font_bold();
        textSize(20.0f);
        fill(textColor);
        text(wave > 0 ? std::format("Welle {}", wave) : std::string("Alle Typen"), header.left, header.top + 20.0f);

        font_medium();
        textSize(12.0f);
        textAlign(TextAlignment::topRight);
        fill(mutedTextColor);
        text(wave > 0 ? "Anteil an der laufenden Welle" : "Freischaltung nach Welle", header.left + header.width, header.top + 26.0f);

        withClip(layout.list.left - 2.0f, layout.list.top, layout.list.width + 4.0f, layout.list.height, [&] {
            float y = layout.list.top - bestiary.scroll.offset;
            for (const SpawnEntry& entry : enemy_roster()) {
                show_row(rect2f {layout.list.left, y, layout.list.width, rowHeight}, entry, wave);
                y += rowHeight + rowGap;
            }
        });
        scroll_list_show_bar(bestiary.scroll, layout.list, layout.contentHeight);

        button(layout.close, "Schließen", accentColor);
    });
}
