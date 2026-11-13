#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

#include "meters.hpp"

struct Tower
{
    p5::float2 position;
    float radius;

    // health is stored as damage taken, so raising the max health heals by the same amount
    float damageTaken;
    p5::animation::ValueTweenTransition<float> hitFlash;
};

Tower tower_create(const p5::float2& position, float radius);
float tower_health(const Tower& tower, float maxHealth);
bool tower_is_destroyed(const Tower& tower, float maxHealth);
void tower_damage(Tower& tower, float damage);
void tower_regenerate(Tower& tower, float amount);
void tower_update(Tower& tower, float deltaTime);
// the health fraction rides along as a ring around the tower, it only appears once the tower is hurt
void tower_show(const Tower& tower, const Meters& range, float healthFraction);
