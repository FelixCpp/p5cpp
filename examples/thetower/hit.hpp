#pragma once

struct Hit
{
    float damage;
    bool isCrit;
    bool reserved; // the damage was reserved on the target when the hit was created
};
