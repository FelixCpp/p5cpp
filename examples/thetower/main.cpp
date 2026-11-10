#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

using namespace p5;
using namespace p5::animation;

#include "easy_enemy.hpp"
#include "enemy_movement.hpp"
#include "enemy_size.hpp"
#include "game_state.hpp"
#include "meters.hpp"
#include "tower.hpp"

struct TheTower : Sketch
{
    Tower tower = {
        .position = float2 {300.0f, 300.0f},
    };

    EasyEnemy enemy = {
        .size = EnemySize {
            .size = valueTween(0.0f, 25.0f, 2.0f, curves::easeInOutCubic),
        },
        .movement = EnemyMovement {
            .position = {100.0f, 100.0f},
            .velocity = {},
            .acceleration = {},
            .maxSpeed = 80.0f,
            .maxForce = 150.0f,
        },
    };

    GameState stats = {
        .towerRange = Meters {
            .value = 150.0f,
        },
    };

    void setup() override
    {
        setWindowSize(600, 600);
        setWindowResizable(false);
    }

    void draw() override
    {
        const float deltaTime = getDeltaTime();
        easy_enemy_seek(enemy, tower.position);
        easy_enemy_update(enemy, deltaTime);

        background(rgba(31));

        tower_show(tower, stats.towerRange);
        easy_enemy_show(enemy);
    }
};

SketchSpec p5::createSpec()
{
    return {
        .plugins = {},
        .sketch = [] {
            return std::make_unique<TheTower>();
        }
    };
}
