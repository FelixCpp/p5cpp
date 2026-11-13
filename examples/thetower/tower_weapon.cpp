#include "tower_weapon.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <span>

#include "palette.hpp"

using namespace p5;
using namespace p5::animation;

namespace
{
    // excluded enemies already got a bullet from the same shot, they need not be doomed yet
    Enemy* find_target(std::vector<Enemy>& enemies, const Tower& tower, const TowerStats& stats, const float range, const std::span<const EnemyId> excluded)
    {
        Enemy* target = nullptr;
        float earliestArrival = std::numeric_limits<float>::infinity();

        for (Enemy& enemy : enemies) {
            if (enemy_health_is_doomed(enemy.core.health, stats.executeThreshold) || std::ranges::contains(excluded, enemy.core.id)) {
                continue;
            }
            if (distanceSquared(tower.position, enemy.core.movement.position) > range * range) {
                continue;
            }

            const float arrival = enemy_core_time_to_reach(enemy.core, tower);
            if (arrival < earliestArrival) {
                earliestArrival = arrival;
                target = &enemy;
            }
        }

        return target;
    }

    // time until a bullet fired from origin meets a target moving with constant velocity
    float intercept_time(const float2& origin, const float2& targetPosition, const float2& targetVelocity, const float bulletSpeed)
    {
        const float2 offset = targetPosition - origin;

        const float a = dot(targetVelocity, targetVelocity) - bulletSpeed * bulletSpeed;
        const float b = 2.0f * dot(offset, targetVelocity);
        const float c = dot(offset, offset);

        // a target at least as fast as the bullet has no reliable intercept, fall back to its current distance
        if (a >= 0.0f) {
            return std::sqrt(c) / bulletSpeed;
        }

        // a < 0 and c >= 0 guarantee a non-negative discriminant with exactly one positive root
        const float discriminant = b * b - 4.0f * a * c;
        return (-b - std::sqrt(discriminant)) / (2.0f * a);
    }

    void fire_at(Enemy& target, const TowerStats& stats, const Tower& tower, std::vector<Bullet>& bullets)
    {
        // rolled when firing, so the reserved damage already includes the crit
        const bool isCrit = p5::random() < stats.critChance;
        const EnemyMovement& movement = target.core.movement;
        const float flightTime = intercept_time(tower.position, movement.position, movement.velocity, stats.bulletSpeed);
        const float2 impact = movement.position + movement.velocity * flightTime;

        const Hit hit = {
            .damage = isCrit ? stats.damage * stats.critMultiplier : stats.damage,
            .isCrit = isCrit,
            .reserved = true,
            .bouncesLeft = p5::random() < stats.bounceChance ? stats.bounceCount : 0,
            .direction = fixedLength(impact - tower.position, 1.0f),
            .knockback = stats.knockback.toPixels(),
            .slowFactor = stats.slowFactor,
            .slowDuration = stats.slowDuration,
        };

        enemy_health_reserve(target.core.health, hit.damage);
        bullets.push_back(Bullet {
            .target = target.core.id,
            .hit = hit,
            .origin = tower.position,
            .impact = impact,
            .position = tower.position,
            .flightTime = flightTime,
            .elapsed = 0.0f,
            .kind = BulletKind::shot,
            .chain = {},
        });
    }
}

void tower_weapon_update(TowerWeapon& weapon, const TowerStats& stats, const Tower& tower, std::vector<Enemy>& enemies, std::vector<Bullet>& bullets, const float deltaTime)
{
    weapon.cooldown -= deltaTime;
    weapon.rapidFireLeft = std::max(weapon.rapidFireLeft - deltaTime, 0.0f);
    weapon.shotFlash.advance(deltaTime);

    const float range = stats.range.toPixels();

    while (weapon.cooldown <= 0.0f) {
        Enemy* target = find_target(enemies, tower, stats, range, {});
        if (target == nullptr) {
            weapon.cooldown = 0.0f;
            return;
        }

        const int targetCount = p5::random() < stats.multishotChance ? stats.multishotTargets : 1;
        std::vector<EnemyId> targeted;
        targeted.reserve(targetCount);

        // every bullet of a shot goes to a different enemy, a shot with fewer enemies in range just fires fewer bullets
        while (target != nullptr) {
            fire_at(*target, stats, tower, bullets);
            targeted.push_back(target->core.id);

            if (static_cast<int>(targeted.size()) >= targetCount) {
                break;
            }
            target = find_target(enemies, tower, stats, range, targeted);
        }

        weapon.shotFlash = valueTween(1.0f, 0.0f, 0.12f, curves::easeOutQuad);

        // shots of a running burst cannot start a new one, otherwise a high attack speed keeps it going forever
        if (weapon.rapidFireLeft <= 0.0f && p5::random() < stats.rapidFireChance) {
            weapon.rapidFireLeft = stats.rapidFireDuration;
        }

        weapon.cooldown += weapon.rapidFireLeft > 0.0f ? stats.fireInterval / stats.rapidFireFactor : stats.fireInterval;
    }
}

void tower_weapon_show(const TowerWeapon& weapon, const TowerStats& stats, const Tower& tower)
{
    // a burst shows as a core that flickers hot, it cools down over the last part of its duration
    const float heat = std::min(weapon.rapidFireLeft / std::min(stats.rapidFireDuration, 0.4f), 1.0f);
    const float flicker = 0.55f + 0.45f * std::sin(static_cast<float>(getGlobalTime()) * 34.0f);

    // every single shot punches a flash on top of whatever the burst is already glowing
    const float glow = std::max(weapon.shotFlash.value(), heat * flicker);
    if (glow <= 0.01f) {
        return;
    }

    const color_t color = lerpColor(accentColor, heatColor, heat);

    with([&] {
        blendMode(BlendMode::additive);
        noStroke();

        fill(withOpacity(color, 0.09f * glow));
        for (int i = 3; i >= 1; --i) {
            circle(tower.position.x, tower.position.y, tower.radius * (0.15f + 0.15f * glow) * static_cast<float>(i));
        }

        fill(withOpacity(lerpColor(color, rgba(255), 0.5f), 0.8f * glow));
        circle(tower.position.x, tower.position.y, tower.radius * 0.1f * (0.8f + 0.7f * glow));
    });
}
