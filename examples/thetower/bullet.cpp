#include "bullet.hpp"

using namespace p5;

void bullets_update(std::vector<Bullet>& bullets, std::vector<Enemy>& enemies, GameEvents& events, const float deltaTime)
{
    std::erase_if(bullets, [&enemies, &events, deltaTime](Bullet& bullet) {
        Enemy* target = enemy_find(enemies, bullet.target);
        if (target == nullptr) {
            return true;
        }

        bullet.elapsed += deltaTime;
        if (bullet.elapsed >= bullet.flightTime) {
            enemy_core_apply_hit(target->core, bullet.hit, events);
            return true;
        }

        // fly straight at constant speed towards the predicted impact and only bend onto the real
        // position towards the end, so small deviations of the target never show up as speed changes
        const float progress = bullet.elapsed / bullet.flightTime;
        const float2 aim = lerp(bullet.impact, target->core.movement.position, progress * progress);
        bullet.position = lerp(bullet.origin, aim, progress);
        return false;
    });
}

void bullet_show(const Bullet& bullet)
{
    with([&bullet] {
        noFill();
        stroke(rgba(60, 180, 220));
        strokeWeight(6.0f);
        point(bullet.position.x, bullet.position.y);
    });
}
