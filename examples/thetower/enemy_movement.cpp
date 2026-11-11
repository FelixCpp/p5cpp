#include "enemy_movement.hpp"

#include <algorithm>

using namespace p5;

void enemy_movement_update(EnemyMovement& movement, float deltaTime)
{
    movement.acceleration = limited(movement.acceleration, movement.maxForce);
    movement.velocity += movement.acceleration * deltaTime;
    movement.velocity = limited(movement.velocity, movement.maxSpeed);
    movement.acceleration = float2 {};

    movement.position += movement.velocity * deltaTime;
}

float enemy_movement_time_to_reach(const EnemyMovement& movement, const p5::float2& target, const float reachDistance)
{
    const float remaining = std::max(distance(movement.position, target) - reachDistance, 0.0f);
    return remaining / movement.maxSpeed;
}
