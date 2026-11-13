#pragma once

#include <p5cpp/p5cpp.hpp>

#include "enemy_core.hpp"
#include "enemy_shape.hpp"

// what an enemy looks like no matter what it does: the shadow on the ground, the halo around it,
// the body itself and the rings and bars that report its state
struct EnemyBodyLook
{
    p5::color_t color;
    EnemyShape shape;
    float strokeWeight;
    p5::angle_t rotation;

    float extraGlow;  // brightens the halo, e.g. while a shot leaves the body
    float extraScale; // swells the body for the same reason
};

void enemy_body_show(const EnemyCore& core, const EnemyBodyLook& look);
