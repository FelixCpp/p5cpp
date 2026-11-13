#include "enemy_health.hpp"

bool enemy_health_is_doomed(const EnemyHealth& health, const float executeThreshold)
{
    return health.current - health.pendingDamage <= health.maxHealth * executeThreshold;
}

bool enemy_health_is_dead(const EnemyHealth& health)
{
    return health.current <= 0.0f;
}

bool enemy_health_is_executed(const EnemyHealth& health, const float executeThreshold)
{
    return executeThreshold > 0.0f && health.current <= health.maxHealth * executeThreshold;
}

void enemy_health_reserve(EnemyHealth& health, const float damage)
{
    health.pendingDamage += damage;
}

void enemy_health_apply(EnemyHealth& health, const float damage)
{
    health.pendingDamage -= damage;
    health.current -= damage;
}

void enemy_health_apply_unreserved(EnemyHealth& health, const float damage)
{
    health.current -= damage;
}
