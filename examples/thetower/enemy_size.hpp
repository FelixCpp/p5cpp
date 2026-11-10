#pragma once

#include <p5cpp_animation/p5cpp_animation.hpp>

struct EnemySize
{
    p5::animation::ValueTweenTransition<float> size;
};

void enemy_size_update(EnemySize& size, float deltaTime);
