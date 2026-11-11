#pragma once

struct EnemyHealth
{
    float current;
    float pendingDamage;
};

bool enemy_health_is_doomed(const EnemyHealth& health);
bool enemy_health_is_dead(const EnemyHealth& health);
void enemy_health_reserve(EnemyHealth& health, float damage);
void enemy_health_apply(EnemyHealth& health, float damage);
