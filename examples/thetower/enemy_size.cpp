#include "enemy_size.hpp"

void enemy_size_update(EnemySize& size, float deltaTime)
{
    size.size.advance(deltaTime);
}
