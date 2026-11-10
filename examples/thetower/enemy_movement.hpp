#pragma once

#include <p5cpp/p5cpp.hpp>

struct EnemyMovement
{
    p5::float2 position;
    p5::float2 velocity;
    p5::float2 acceleration;

    float maxSpeed;
    float maxForce;
};

void enemy_movement_update(EnemyMovement& movement, float deltaTime);
