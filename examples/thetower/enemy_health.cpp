#include "enemy_health.hpp"

bool enemy_health_is_doomed(const EnemyHealth& health)
{
    return health.current - health.pendingDamage <= 0.0f;
}

bool enemy_health_is_dead(const EnemyHealth& health)
{
    return health.current <= 0.0f;
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
