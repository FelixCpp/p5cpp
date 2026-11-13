#include "enemy_health.hpp"

#include <algorithm>

namespace
{
    // the shield takes what it can, only the rest reaches the health
    void take(EnemyHealth& health, const float damage)
    {
        const float absorbed = std::min(health.shield, damage);
        health.shield -= absorbed;
        health.current -= damage - absorbed;
    }
}

bool enemy_health_is_doomed(const EnemyHealth& health, const float executeThreshold)
{
    // a shield counts towards what is still left to chew through, so the tower keeps firing while it holds
    return health.shield + health.current - health.pendingDamage <= health.maxHealth * executeThreshold;
}

bool enemy_health_is_dead(const EnemyHealth& health)
{
    return health.current <= 0.0f;
}

bool enemy_health_is_executed(const EnemyHealth& health, const float executeThreshold)
{
    return executeThreshold > 0.0f && health.shield <= 0.0f && health.current <= health.maxHealth * executeThreshold;
}

bool enemy_health_has_shield(const EnemyHealth& health)
{
    return health.shield > 0.0f;
}

float enemy_health_shield_fraction(const EnemyHealth& health)
{
    return health.maxShield > 0.0f ? std::clamp(health.shield / health.maxShield, 0.0f, 1.0f) : 0.0f;
}

void enemy_health_reserve(EnemyHealth& health, const float damage)
{
    health.pendingDamage += damage;
}

void enemy_health_apply(EnemyHealth& health, const float damage)
{
    health.pendingDamage -= damage;
    take(health, damage);
}

void enemy_health_apply_unreserved(EnemyHealth& health, const float damage)
{
    take(health, damage);
}
