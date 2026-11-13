#pragma once

#include <p5cpp/p5cpp.hpp>

struct EnemyMovement
{
    p5::float2 position;
    p5::float2 velocity;
    p5::float2 acceleration;

    float maxSpeed;
    float maxForce;

    p5::float2 push;  // knockback, decays on its own and is never steered
    float slowFactor; // 0..1, speed lost while the slow runs
    float slowLeft;   // seconds
};

// max speed the slow leaves the enemy
float enemy_movement_speed(const EnemyMovement& movement);
// the running slow, 0 when none runs
float enemy_movement_slow_factor(const EnemyMovement& movement);
bool enemy_movement_is_pushed(const EnemyMovement& movement);

// shoves the enemy the given distance along the direction, the push fades out over it
void enemy_movement_push(EnemyMovement& movement, const p5::float2& direction, float distance);
// slows never stack, a new one only refreshes the duration
void enemy_movement_slow(EnemyMovement& movement, float factor, float duration);

void enemy_movement_update(EnemyMovement& movement, float deltaTime);
float enemy_movement_time_to_reach(const EnemyMovement& movement, const p5::float2& target, float reachDistance);
