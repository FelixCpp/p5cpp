#include "enemy_movement.hpp"

using namespace p5;

void enemy_movement_update(EnemyMovement& movement, float deltaTime)
{
    movement.acceleration = limited(movement.acceleration, movement.maxForce);
    movement.velocity += movement.acceleration * deltaTime;
    movement.velocity = limited(movement.velocity, movement.maxSpeed);
    movement.acceleration = float2 {};

    movement.position += movement.velocity * deltaTime;
}
