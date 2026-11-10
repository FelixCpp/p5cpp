#include "easy_enemy.hpp"

using namespace p5;

void easy_enemy_seek(EasyEnemy& enemy, const p5::float2& target)
{
    EnemyMovement& movement = enemy.movement;

    const float2 desired = fixedLength(target - movement.position, movement.maxSpeed);
    const float2 steering = limited(desired - movement.velocity, movement.maxForce);

    movement.acceleration += steering;
}

void easy_enemy_update(EasyEnemy& enemy, const float deltaTime)
{
    enemy_size_update(enemy.size, deltaTime);
    enemy_movement_update(enemy.movement, deltaTime);
}

void easy_enemy_show(const EasyEnemy& enemy)
{
    with([&enemy] {
        const float size = enemy.size.size.value();
        const auto [x, y] = enemy.movement.position;

        noFill();
        stroke(rgba(220, 180, 150));
        strokeWeight(3.0f);
        rect(x - size * 0.5f, y - size * 0.5f, size, size);
    });
}
