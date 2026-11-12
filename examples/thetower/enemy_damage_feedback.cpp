#include "enemy_damage_feedback.hpp"

using namespace p5;
using namespace p5::animation;

EnemyDamageFeedback enemy_damage_feedback_create()
{
    return EnemyDamageFeedback {
        .flash = valueTweenJump(0.0f),
    };
}

void enemy_damage_feedback_hit(EnemyDamageFeedback& feedback)
{
    feedback.flash = valueTween(1.0f, 0.0f, 0.25f, curves::easeOutQuad);
}

void enemy_damage_feedback_update(EnemyDamageFeedback& feedback, const float deltaTime)
{
    feedback.flash.advance(deltaTime);
}

color_t enemy_damage_feedback_fill(const EnemyDamageFeedback& feedback, const color_t base)
{
    return withOpacity(base, feedback.flash.value());
}
