#include "bullet.hpp"

#include <algorithm>
#include <array>
#include <utility>

#include "palette.hpp"

using namespace p5;

namespace
{
    constexpr float boltSpeed = 1200.0f;

    constexpr color_t boltColor = rgba(240, 230, 120);

    // nearest enemy around the one just hit, picked only when the bolt jumps so it sees the current state
    Enemy* find_bounce_target(std::vector<Enemy>& enemies, const std::vector<EnemyId>& chain, const float2& position, const float range)
    {
        Enemy* target = nullptr;
        float nearest = range * range;

        for (Enemy& enemy : enemies) {
            if (std::ranges::contains(chain, enemy.core.id) || enemy_health_is_doomed(enemy.core.health)) {
                continue;
            }

            const float distance = distanceSquared(position, enemy.core.movement.position);
            if (distance <= nearest) {
                nearest = distance;
                target = &enemy;
            }
        }

        return target;
    }

    void bounce(const Bullet& bullet, const Enemy& hitEnemy, std::vector<Enemy>& enemies, const TowerStats& stats, std::vector<Bullet>& bolts)
    {
        const float2 from = hitEnemy.core.movement.position;

        std::vector<EnemyId> chain = bullet.chain;
        chain.push_back(hitEnemy.core.id);

        Enemy* target = find_bounce_target(enemies, chain, from, stats.bounceRange.toPixels());
        if (target == nullptr) {
            return;
        }

        Hit hit = bullet.hit;
        --hit.bouncesLeft;

        // the bolt is so fast that leading the target by its current velocity is close enough
        const EnemyMovement& movement = target->core.movement;
        const float flightTime = distance(from, movement.position) / boltSpeed;

        enemy_health_reserve(target->core.health, hit.damage);
        bolts.push_back(Bullet {
            .target = target->core.id,
            .hit = hit,
            .origin = from,
            .impact = movement.position + movement.velocity * flightTime,
            .position = from,
            .flightTime = flightTime,
            .elapsed = 0.0f,
            .kind = BulletKind::bolt,
            .chain = std::move(chain),
        });
    }
}

namespace
{
    void show_shot(const Bullet& bullet)
    {
        const bool crit = bullet.hit.isCrit;
        const color_t color = crit ? critColor : accentColor;
        const float size = crit ? 1.4f : 1.0f;

        // the trail points back along the flight path and grows in during the first pixels
        const float2 travelled = bullet.position - bullet.origin;
        const float travelledLength = length(travelled);
        if (travelledLength > 0.5f) {
            constexpr int segments = 6;
            const float2 tail = bullet.position - travelled / travelledLength * std::min(travelledLength, 22.0f * size);

            for (int i = 0; i < segments; ++i) {
                const float t0 = static_cast<float>(i) / segments;
                const float t1 = static_cast<float>(i + 1) / segments;
                const float2 a = lerp(tail, bullet.position, t0);
                const float2 b = lerp(tail, bullet.position, t1);

                stroke(withOpacity(color, 0.55f * t1));
                strokeWeight((1.0f + 3.0f * t1) * size);
                line(a.x, a.y, b.x, b.y);
            }
        }

        stroke(withOpacity(color, 0.25f));
        strokeWeight(14.0f * size);
        point(bullet.position.x, bullet.position.y);

        stroke(lerpColor(color, rgba(255), 0.6f));
        strokeWeight(5.0f * size);
        point(bullet.position.x, bullet.position.y);
    }

    // jagged line from the enemy it jumped off to its tip, re-rolled every frame so it flickers
    void show_bolt(const Bullet& bullet)
    {
        const float2 delta = bullet.position - bullet.origin;
        const float distance = length(delta);
        if (distance < 1.0f) {
            return;
        }

        constexpr int segments = 6;
        const float2 normal = float2 {-delta.y, delta.x} / distance;

        std::array<float2, segments + 1> points;
        for (int i = 0; i <= segments; ++i) {
            const float t = static_cast<float>(i) / segments;
            const bool end = i == 0 || i == segments;
            points[i] = lerp(bullet.origin, bullet.position, t) + normal * (end ? 0.0f : p5::random(-6.0f, 6.0f));
        }

        for (const auto& [weight, alpha] : {std::pair {7.0f, 0.25f}, std::pair {2.0f, 1.0f}}) {
            stroke(withOpacity(boltColor, alpha));
            strokeWeight(weight);
            for (int i = 0; i < segments; ++i) {
                line(points[i].x, points[i].y, points[i + 1].x, points[i + 1].y);
            }
        }
    }
}

void bullets_update(std::vector<Bullet>& bullets, std::vector<Enemy>& enemies, const TowerStats& stats, GameEvents& events, const float deltaTime)
{
    // bolts spawned while iterating are added afterwards, they start flying next frame
    std::vector<Bullet> bolts;

    std::erase_if(bullets, [&enemies, &stats, &events, &bolts, deltaTime](Bullet& bullet) {
        // a target killed earlier this frame, e.g. by thorns, is only removed at the end of the frame,
        // the bullet must treat it as gone, otherwise it would still bounce on from the corpse
        Enemy* target = enemy_find(enemies, bullet.target);
        if (target == nullptr || enemy_health_is_dead(target->core.health)) {
            return true;
        }

        bullet.elapsed += deltaTime;
        if (bullet.elapsed >= bullet.flightTime) {
            enemy_core_apply_hit(target->core, bullet.hit, events);
            if (bullet.hit.bouncesLeft > 0) {
                bounce(bullet, *target, enemies, stats, bolts);
            }
            return true;
        }

        // fly straight at constant speed towards the predicted impact and only bend onto the real
        // position towards the end, so small deviations of the target never show up as speed changes
        const float progress = bullet.elapsed / bullet.flightTime;
        const float2 aim = lerp(bullet.impact, target->core.movement.position, progress * progress);
        bullet.position = lerp(bullet.origin, aim, progress);
        return false;
    });

    bullets.insert(bullets.end(), bolts.begin(), bolts.end());
}

void bullet_show(const Bullet& bullet)
{
    with([&bullet] {
        // additive blending lets overlapping trails and glows brighten each other
        blendMode(BlendMode::additive);
        strokeCap(StrokeCap::round);
        noFill();

        switch (bullet.kind) {
            case BulletKind::shot: show_shot(bullet); break;
            case BulletKind::bolt: show_bolt(bullet); break;
        }
    });
}
