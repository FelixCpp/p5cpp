#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

struct EnemyDamageFeedback
{
    p5::animation::ValueTweenTransition<float> flash;
};

EnemyDamageFeedback enemy_damage_feedback_create();
void enemy_damage_feedback_hit(EnemyDamageFeedback& feedback);
void enemy_damage_feedback_update(EnemyDamageFeedback& feedback, float deltaTime);

p5::color_t enemy_damage_feedback_fill(const EnemyDamageFeedback& feedback, p5::color_t base);
