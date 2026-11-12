#include "bullet.hpp"

#include <limits>

using namespace p5;

namespace
{
    constexpr float boltSpeed = 1200.0f;

    // nearest enemy around the one just hit, picked only when the bolt jumps so it sees the current state
    Enemy* find_bounce_target(std::vector<Enemy>& enemies, const EnemyId from, const float2& position, const float range)
    {
        Enemy* target = nullptr;
        float nearest = range * range;

        for (Enemy& enemy : enemies) {
            if (enemy.core.id == from || enemy_health_is_doomed(enemy.core.health)) {
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
        Enemy* target = find_bounce_target(enemies, hitEnemy.core.id, from, stats.bounceRange.toPixels());
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
        });
    }
}

void bullets_update(std::vector<Bullet>& bullets, std::vector<Enemy>& enemies, const TowerStats& stats, GameEvents& events, const float deltaTime)
{
    // bolts spawned while iterating are added afterwards, they start flying next frame
    std::vector<Bullet> bolts;

    std::erase_if(bullets, [&enemies, &stats, &events, &bolts, deltaTime](Bullet& bullet) {
        Enemy* target = enemy_find(enemies, bullet.target);
        if (target == nullptr) {
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
        noFill();
        switch (bullet.kind) {
            case BulletKind::shot:
                stroke(rgba(60, 180, 220));
                strokeWeight(6.0f);
                point(bullet.position.x, bullet.position.y);
                break;
            case BulletKind::bolt:
                // a streak from the enemy it jumped off to its current tip
                stroke(rgba(240, 230, 120));
                strokeWeight(3.0f);
                line(bullet.origin.x, bullet.origin.y, bullet.position.x, bullet.position.y);
                break;
        }
    });
}
