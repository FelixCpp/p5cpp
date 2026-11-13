#include "enemy_shape.hpp"

using namespace p5;

void enemy_shape_draw(const EnemyShape shape, const p5::float2& center, const float size)
{
    switch (shape) {
        case EnemyShape::square: {
            rect(center.x - size * 0.5f, center.y - size * 0.5f, size, size);
            break;
        }
        case EnemyShape::triangle: {
            triangle(center.x + size * 0.62f, center.y, center.x - size * 0.45f, center.y - size * 0.52f, center.x - size * 0.45f, center.y + size * 0.52f);
            break;
        }
        case EnemyShape::hexagon: {
            strokeJoin(StrokeJoin::round);
            beginShape(ShapeMode::path);
            for (size_t i = 0; i < 6; ++i) {
                const float2 corner = center + fromAngle(degrees(60.0f * i), size * 0.58f);
                vertex(corner.x, corner.y);
            }
            endShape(true);
            break;
        }
    }
}
