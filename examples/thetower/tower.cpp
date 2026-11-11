#include "tower.hpp"

#include <algorithm>

using namespace p5;
using namespace p5::animation;

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

void tower_show(const Tower& tower, const Meters& range)
{
    with([&tower, &range] {
        static constexpr color_t color = rgba(60, 180, 220);
        static constexpr color_t hitColor = rgba(230, 80, 80);

        const float flash = tower.hitFlash.value();

        fill(rgba(hitColor.r, hitColor.g, hitColor.b, static_cast<int32_t>(160.0f * flash)));
        stroke(lerpColor(color, hitColor, flash));
        strokeWeight(3.0f);

        strokeJoin(StrokeJoin::round);
        beginShape(ShapeMode::path);
        for (size_t i = 0; i < 6; ++i) {
            const float t = i / 6.0f * TAU;
            const float x = tower.position.x + std::cos(t) * tower.radius;
            const float y = tower.position.y + std::sin(t) * tower.radius;

            vertex(x, y);
        }
        endShape(true);

        noFill();
        stroke(rgba(71.0f));
        strokeWeight(4.0f);
        circle(tower.position.x, tower.position.y, range.toPixels());
    });
}
