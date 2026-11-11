#include "enemy_spawner.hpp"

using namespace p5;

void enemy_spawner_update(EnemySpawner& spawner, std::vector<Enemy>& enemies, const float deltaTime)
{
    spawner.elapsed += deltaTime;

    while (spawner.elapsed >= spawner.interval) {
        spawner.elapsed -= spawner.interval;

        const float angle = random(TAU);
        const float2 offset = float2 {std::cos(angle), std::sin(angle)} * spawner.spawnDistance;
        enemies.push_back(melee_enemy_create(spawner.nextId++, spawner.center + offset));
    }
}
