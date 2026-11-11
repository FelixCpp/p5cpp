#include "tower_weapon.hpp"

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

        enemy_health_reserve(target->core.health, hit.damage);
        bullets.push_back(Bullet {
            .target = target->core.id,
            .hit = hit,
            .origin = tower.position,
            .position = tower.position,
            .flightTime = distance(tower.position, target->core.movement.position) / stats.bulletSpeed,
            .elapsed = 0.0f,
        });

        weapon.cooldown += stats.fireInterval;
    }
}
