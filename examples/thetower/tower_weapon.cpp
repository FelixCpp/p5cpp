#include "tower_weapon.hpp"

#include <cmath>
#include <limits>

using namespace p5;

namespace
{
    Enemy* find_target(std::vector<Enemy>& enemies, const Tower& tower, const float range)
    {
        Enemy* target = nullptr;
        float earliestArrival = std::numeric_limits<float>::infinity();

        for (Enemy& enemy : enemies) {
            if (enemy_health_is_doomed(enemy.core.health)) {
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
}

void tower_weapon_update(TowerWeapon& weapon, const TowerStats& stats, const Tower& tower, std::vector<Enemy>& enemies, std::vector<Bullet>& bullets, const float deltaTime)
{
    weapon.cooldown -= deltaTime;

    while (weapon.cooldown <= 0.0f) {
        Enemy* target = find_target(enemies, tower, stats.range.toPixels());
        if (target == nullptr) {
            weapon.cooldown = 0.0f;
            return;
        }

        const Hit hit = {
            .damage = stats.damage,
        };

        const EnemyMovement& movement = target->core.movement;
        const float flightTime = intercept_time(tower.position, movement.position, movement.velocity, stats.bulletSpeed);

        enemy_health_reserve(target->core.health, hit.damage);
        bullets.push_back(Bullet {
            .target = target->core.id,
            .hit = hit,
            .origin = tower.position,
            .impact = movement.position + movement.velocity * flightTime,
            .position = tower.position,
            .flightTime = flightTime,
            .elapsed = 0.0f,
        });

        weapon.cooldown += stats.fireInterval;
    }
}
