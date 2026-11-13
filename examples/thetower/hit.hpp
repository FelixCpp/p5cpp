#pragma once

#include <p5cpp/p5cpp.hpp>

struct Hit
{
    float damage;
    bool isCrit;
    bool reserved; // the damage was reserved on the target when the hit was created
    int bouncesLeft; // jumps on to the next enemy after landing

    p5::float2 direction; // unit vector the hit travelled along, zero when it pushes nothing
    float knockback;      // pixels a weightless enemy is shoved along the direction
    float slowFactor;     // 0..1, speed the enemy loses for the slow duration
    float slowDuration;   // seconds, a new hit only refreshes it
};
