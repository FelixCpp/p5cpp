#include "enemy_shot.hpp"

#include <algorithm>

#include "palette.hpp"

using namespace p5;

namespace
{
    constexpr float shotRadius = 4.0f;
    constexpr float trailLength = 18.0f;

    float progress(const EnemyShot& shot)
    {
        return shot.flightTime > 0.0f ? std::clamp(shot.elapsed / shot.flightTime, 0.0f, 1.0f) : 1.0f;
    }

    float2 position(const EnemyShot& shot)
    {
        return lerp(shot.origin, shot.target, progress(shot));
    }
}

void enemy_shots_spawn(std::vector<EnemyShot>& shots, const Tower& tower, const GameEvents& events)
{
    for (const EnemyShotEvent& event : events.enemyShots) {
        // the shot aims at the tower's edge, so it lands where the hull is instead of in its middle
        const float2 target = tower.position + fixedLength(event.origin - tower.position, tower.radius);
        const float distance = length(target - event.origin);

        shots.push_back(EnemyShot {
            .shooter = event.shooter,
            .origin = event.origin,
            .target = target,
            .color = event.color,
            .damage = event.damage,
            .flightTime = event.speed > 0.0f ? distance / event.speed : 0.0f,
            .elapsed = 0.0f,
        });
    }
}

void enemy_shots_update(std::vector<EnemyShot>& shots, GameEvents& events, const float deltaTime)
{
    for (EnemyShot& shot : shots) {
        shot.elapsed += deltaTime;
        if (shot.elapsed < shot.flightTime) {
            continue;
        }

        events.towerHits.push_back(TowerHitEvent {
            .attacker = shot.shooter,
            .position = shot.target,
            .damage = shot.damage,
        });
    }

    std::erase_if(shots, [](const EnemyShot& shot) { return shot.elapsed >= shot.flightTime; });
}

void enemy_shots_show(const std::vector<EnemyShot>& shots)
{
    with([&] {
        strokeCap(StrokeCap::round);

        for (const EnemyShot& shot : shots) {
            const float2 at = position(shot);
            const float2 back = at - fixedLength(shot.target - shot.origin, trailLength);

            // a trail behind the head, the same way the tower's own shots read as moving
            stroke(withOpacity(shot.color, 0.4f));
            strokeWeight(shotRadius * 0.8f);
            line(back.x, back.y, at.x, at.y);

            noStroke();
            fill(shot.color);
            circle(at.x, at.y, shotRadius);
            fill(withOpacity(rgba(255), 0.55f));
            circle(at.x, at.y, shotRadius * 0.45f);
        }
    });
}
