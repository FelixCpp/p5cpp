#include "enemy_movement.hpp"

#include <algorithm>
#include <cmath>

using namespace p5;

namespace
{
    // how fast a knockback bleeds off, per second
    constexpr float pushDecay = 9.0f;
    // below that speed the push is over
    constexpr float pushMinSpeed = 4.0f;
}

float enemy_movement_speed(const EnemyMovement& movement)
{
    return movement.maxSpeed * (1.0f - enemy_movement_slow_factor(movement));
}

float enemy_movement_slow_factor(const EnemyMovement& movement)
{
    return movement.slowLeft > 0.0f ? movement.slowFactor : 0.0f;
}

bool enemy_movement_is_pushed(const EnemyMovement& movement)
{
    return lengthSquared(movement.push) > pushMinSpeed * pushMinSpeed;
}

void enemy_movement_push(EnemyMovement& movement, const p5::float2& direction, const float distance)
{
    // a push of speed v covers v / pushDecay before it has faded, so the distance sets the speed
    const float speed = distance * pushDecay;
    // hit after hit the pushes would add up to any speed, two shoves worth is the limit
    movement.push = limited(movement.push + fixedLength(direction, speed), speed * 2.0f);
}

void enemy_movement_slow(EnemyMovement& movement, const float factor, const float duration)
{
    movement.slowFactor = factor;
    movement.slowLeft = std::max(movement.slowLeft, duration);
}

void enemy_movement_update(EnemyMovement& movement, float deltaTime)
{
    movement.slowLeft = std::max(movement.slowLeft - deltaTime, 0.0f);

    movement.acceleration = limited(movement.acceleration, movement.maxForce);
    movement.velocity += movement.acceleration * deltaTime;
    movement.velocity = limited(movement.velocity, enemy_movement_speed(movement));
    movement.acceleration = float2 {};

    movement.position += (movement.velocity + movement.push) * deltaTime;
    movement.push *= std::exp(-pushDecay * deltaTime);
    if (!enemy_movement_is_pushed(movement)) {
        movement.push = float2 {};
    }
}

float enemy_movement_time_to_reach(const EnemyMovement& movement, const p5::float2& target, const float reachDistance)
{
    const float remaining = std::max(distance(movement.position, target) - reachDistance, 0.0f);
    return remaining / enemy_movement_speed(movement);
}
