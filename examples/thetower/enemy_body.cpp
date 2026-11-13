#include "enemy_body.hpp"

#include <algorithm>

#include "palette.hpp"

using namespace p5;

namespace
{
    // a ring that collapses onto the enemy while it grows to its full size
    void show_spawn_ring(const EnemyCore& core, const color_t color)
    {
        const float grown = core.size.size.progress();
        if (grown >= 1.0f) {
            return;
        }

        const auto [x, y] = core.movement.position;
        const float left = 1.0f - grown;

        noFill();
        stroke(withOpacity(color, 0.45f * left * left));
        strokeWeight(1.5f);
        circle(x, y, core.size.size.value() * 0.5f + 16.0f * left);
    }

    // the shield closes clockwise from the top and is gone for good once it breaks
    void show_shield(const EnemyCore& core, const float size, const color_t color)
    {
        if (!enemy_health_has_shield(core.health)) {
            return;
        }

        const float fraction = enemy_health_shield_fraction(core.health);
        const float radius = size * 0.5f + 7.0f;
        const auto [x, y] = core.movement.position;
        const angle_t start = degrees(-90.0f);

        noFill();
        strokeCap(StrokeCap::round);

        stroke(withOpacity(color, 0.15f));
        strokeWeight(2.5f);
        circle(x, y, radius);

        stroke(lerpColor(color, rgba(255), 0.35f));
        strokeWeight(2.5f);
        arc(x, y, radius, radius, start, start + degrees(360.0f * fraction));
    }

    // only a hurt enemy carries a bar, an untouched crowd stays clean
    void show_health_bar(const EnemyCore& core, const float size)
    {
        const EnemyHealth& health = core.health;
        if (health.maxHealth <= 0.0f || health.current >= health.maxHealth) {
            return;
        }

        const float fraction = std::clamp(health.current / health.maxHealth, 0.0f, 1.0f);
        const float width = std::max(size * 0.9f, 14.0f);
        const float left = core.movement.position.x - width * 0.5f;
        const float top = core.movement.position.y + size * 0.5f + 7.0f;

        noStroke();
        fill(withOpacity(shadowColor, 0.55f));
        rect(left - 1.0f, top - 1.0f, width + 2.0f, 5.0f);
        fill(lerpColor(dangerColor, healthColor, fraction));
        rect(left, top, width * fraction, 3.0f);
    }
}

void enemy_body_show(const EnemyCore& core, const EnemyBodyLook& look)
{
    const float size = core.size.size.value();
    const auto [x, y] = core.movement.position;
    const float flash = core.damageFeedback.flash.value();

    // a slowed enemy is tinted towards frost, the stronger the slow the colder it looks
    const color_t color = lerpColor(look.color, slowColor, enemy_movement_slow_factor(core.movement));

    with([&] {
        noStroke();
        fill(withOpacity(shadowColor, 0.4f));
        ellipse(x, y + size * 0.44f, size * 0.46f, size * 0.18f);
    });

    with([&] {
        // a few layers instead of one disc, otherwise the halo shows its edge
        blendMode(BlendMode::additive);
        noStroke();
        fill(withOpacity(color, 0.035f + 0.05f * look.extraGlow));
        for (int i = 4; i >= 1; --i) {
            circle(x, y, size * 0.22f * static_cast<float>(i));
        }
    });

    with([&] {
        translate(x, y);
        rotate(look.rotation);
        // a hit punches the body a little bigger for as long as the flash lasts
        const float punch = 1.0f + 0.22f * flash + look.extraScale;
        scale(punch, punch);

        fill(lerpColor(rgba(20, 22, 28), color, 0.28f));
        stroke(color);
        strokeWeight(look.strokeWeight);
        enemy_shape_draw(look.shape, float2 {}, size);

        if (flash > 0.0f) {
            noStroke();
            fill(withOpacity(rgba(255), 0.7f * flash));
            enemy_shape_draw(look.shape, float2 {}, size);
        }
    });

    with([&] { show_spawn_ring(core, color); });
    with([&] { show_shield(core, size, color); });
    with([&] { show_health_bar(core, size); });
}
