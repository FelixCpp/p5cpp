#include "tower.hpp"

#include <algorithm>
#include <cmath>

#include "palette.hpp"

using namespace p5;
using namespace p5::animation;

namespace
{
    void hexagon(const float2& center, const float radius, const angle_t rotation)
    {
        strokeJoin(StrokeJoin::round);
        beginShape(ShapeMode::path);
        for (size_t i = 0; i < 6; ++i) {
            const float2 corner = center + fromAngle(rotation + degrees(60.0f * i), radius);
            vertex(corner.x, corner.y);
        }
        endShape(true);
    }

    void show_range(const Tower& tower, const Meters& range)
    {
        noFill();
        stroke(withOpacity(accentColor, 0.22f));
        strokeWeight(1.5f);
        strokeDashPattern({6.0f, 10.0f});
        circle(tower.position.x, tower.position.y, range.toPixels());
    }

    void show_glow(const Tower& tower, const float flash)
    {
        constexpr int layers = 10;

        blendMode(BlendMode::additive);
        noStroke();
        fill(withOpacity(lerpColor(accentColor, dangerColor, flash), 0.03f + 0.05f * flash));

        for (int i = layers; i >= 1; --i) {
            circle(tower.position.x, tower.position.y, tower.radius * 1.6f * static_cast<float>(i) / layers);
        }
    }

    void show_body(const Tower& tower, const float flash)
    {
        const color_t edge = lerpColor(accentColor, dangerColor, flash);
        const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(getGlobalTime()) * 1.6f);

        noStroke();
        fill(withOpacity(shadowColor, 0.45f));
        hexagon(tower.position + float2 {0.0f, 4.0f}, tower.radius, degrees(0.0f));

        fill(lerpColor(rgba(26, 30, 38), dangerColor, 0.55f * flash));
        stroke(edge);
        strokeWeight(3.0f);
        hexagon(tower.position, tower.radius, degrees(0.0f));

        // a second hexagon turns slowly inside the shell, so the tower never looks completely still
        noFill();
        stroke(withOpacity(edge, 0.35f));
        strokeWeight(1.5f);
        hexagon(tower.position, tower.radius * 0.62f, radians(static_cast<float>(getGlobalTime()) * 0.25f));

        noStroke();
        fill(withOpacity(edge, 0.55f + 0.3f * pulse));
        circle(tower.position.x, tower.position.y, tower.radius * (0.17f + 0.02f * pulse));
    }

    // the ring closes clockwise from the top and stays hidden while the tower is untouched
    void show_health_ring(const Tower& tower, const float healthFraction)
    {
        if (healthFraction >= 0.999f) {
            return;
        }

        const float radius = tower.radius * 1.3f;
        const angle_t start = degrees(-90.0f);

        noFill();
        strokeCap(StrokeCap::round);

        stroke(withOpacity(rgba(255), 0.08f));
        strokeWeight(3.0f);
        circle(tower.position.x, tower.position.y, radius);

        stroke(lerpColor(dangerColor, healthColor, healthFraction));
        strokeWeight(3.0f);
        arc(tower.position.x, tower.position.y, radius, radius, start, start + degrees(360.0f * std::clamp(healthFraction, 0.0f, 1.0f)));
    }

    // every strike sends a ring outwards, it is gone before the next one lands
    void show_hit_ring(const Tower& tower, const float flash)
    {
        if (flash <= 0.0f) {
            return;
        }

        const float grown = 1.0f - flash;

        blendMode(BlendMode::additive);
        noFill();
        stroke(withOpacity(dangerColor, 0.7f * flash));
        strokeWeight(1.0f + 4.0f * flash);
        circle(tower.position.x, tower.position.y, tower.radius * (1.0f + 0.9f * grown));
    }
}

Tower tower_create(const p5::float2& position, const float radius)
{
    return Tower {
        .position = position,
        .radius = radius,
        .damageTaken = 0.0f,
        .hitFlash = valueTweenJump(0.0f),
    };
}

float tower_health(const Tower& tower, const float maxHealth)
{
    return std::max(maxHealth - tower.damageTaken, 0.0f);
}

bool tower_is_destroyed(const Tower& tower, const float maxHealth)
{
    return tower.damageTaken >= maxHealth;
}

void tower_damage(Tower& tower, const float damage)
{
    tower.damageTaken += damage;
    tower.hitFlash = valueTween(1.0f, 0.0f, 0.3f, curves::easeOutQuad);
}

void tower_regenerate(Tower& tower, const float amount)
{
    tower.damageTaken = std::max(tower.damageTaken - amount, 0.0f);
}

void tower_update(Tower& tower, const float deltaTime)
{
    tower.hitFlash.advance(deltaTime);
}

void tower_show(const Tower& tower, const Meters& range, const float healthFraction)
{
    const float flash = tower.hitFlash.value();

    with([&] { show_range(tower, range); });
    with([&] { show_glow(tower, flash); });
    with([&] { show_body(tower, flash); });
    with([&] { show_health_ring(tower, healthFraction); });
    with([&] { show_hit_ring(tower, flash); });
}
