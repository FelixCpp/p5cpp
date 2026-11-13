#pragma once

struct EnemyHealth
{
    float current;
    float maxHealth;
    float pendingDamage;
};

// the execute threshold counts as dead, otherwise the tower keeps firing at enemies that are already finished
bool enemy_health_is_doomed(const EnemyHealth& health, float executeThreshold);
bool enemy_health_is_dead(const EnemyHealth& health);
bool enemy_health_is_executed(const EnemyHealth& health, float executeThreshold);
void enemy_health_reserve(EnemyHealth& health, float damage);
void enemy_health_apply(EnemyHealth& health, float damage);
// damage that was never reserved, e.g. thorns
void enemy_health_apply_unreserved(EnemyHealth& health, float damage);
