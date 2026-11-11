#include "enemy.hpp"

#include <algorithm>

namespace
{
    template <typename... Ts>
    struct overloaded : Ts...
    {
        using Ts::operator()...;
    };
}

Enemy* enemy_find(std::vector<Enemy>& enemies, const EnemyId id)
{
    // ids are handed out ascending and removal keeps order, so the vector stays sorted by id
    const auto it = std::ranges::lower_bound(enemies, id, {}, [](const Enemy& enemy) { return enemy.core.id; });
    if (it == enemies.end() || it->core.id != id) {
        return nullptr;
    }
    return &*it;
}

void enemies_remove_dead(std::vector<Enemy>& enemies)
{
    std::erase_if(enemies, [](const Enemy& enemy) {
        return enemy_health_is_dead(enemy.core.health);
    });
}

void enemy_update(Enemy& enemy, const Tower& tower, GameEvents& events, const float deltaTime)
{
    std::visit(
        overloaded {
            [&](MeleeBehavior& behavior) { melee_enemy_update(enemy.core, behavior, tower, events, deltaTime); },
        },
        enemy.behavior
    );
}

void enemy_show(const Enemy& enemy)
{
    std::visit(
        overloaded {
            [&](const MeleeBehavior& behavior) { melee_enemy_show(enemy.core, behavior); },
        },
        enemy.behavior
    );
}
