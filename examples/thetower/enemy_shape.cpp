#include "enemy_shape.hpp"

using namespace p5;

namespace
{
    // corners on one circle, the first one at the given angle
    void polygon(const float2& center, const float radius, const int corners, const angle_t start)
    {
        strokeJoin(StrokeJoin::round);
        beginShape(ShapeMode::path);
        for (int i = 0; i < corners; ++i) {
            const float2 corner = center + fromAngle(start + degrees(360.0f / static_cast<float>(corners) * static_cast<float>(i)), radius);
            vertex(corner.x, corner.y);
        }
        endShape(true);
    }

    // corners alternating between two circles, so the points stick out
    void star(const float2& center, const float radius, const int points, const float innerRatio, const angle_t start)
    {
        strokeJoin(StrokeJoin::round);
        beginShape(ShapeMode::path);
        for (int i = 0; i < points * 2; ++i) {
            const float reach = i % 2 == 0 ? radius : radius * innerRatio;
            const float2 corner = center + fromAngle(start + degrees(180.0f / static_cast<float>(points) * static_cast<float>(i)), reach);
            vertex(corner.x, corner.y);
        }
        endShape(true);
    }

    // an equal armed cross, drawn as one outline so it breaks apart visibly
    void cross(const float2& center, const float radius, const float armRatio)
    {
        const float arm = radius * armRatio;

        strokeJoin(StrokeJoin::round);
        beginShape(ShapeMode::path);
        for (int i = 0; i < 4; ++i) {
            // one quarter of the outline: out along the arm, around its tip, and back in
            const float2 along = fromAngle(degrees(90.0f * static_cast<float>(i)), 1.0f);
            const float2 across = fromAngle(degrees(90.0f * static_cast<float>(i) + 90.0f), 1.0f);

            vertex(center.x + along.x * arm + across.x * arm, center.y + along.y * arm + across.y * arm);
            vertex(center.x + along.x * radius + across.x * arm, center.y + along.y * radius + across.y * arm);
            vertex(center.x + along.x * radius - across.x * arm, center.y + along.y * radius - across.y * arm);
            vertex(center.x + along.x * arm - across.x * arm, center.y + along.y * arm - across.y * arm);
        }
        endShape(true);
    }
}

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
            polygon(center, size * 0.58f, 6, degrees(0.0f));
            break;
        }
        case EnemyShape::diamond: {
            polygon(center, size * 0.62f, 4, degrees(0.0f));
            break;
        }
        case EnemyShape::pentagon: {
            polygon(center, size * 0.62f, 5, degrees(-90.0f));
            break;
        }
        case EnemyShape::octagon: {
            polygon(center, size * 0.56f, 8, degrees(22.5f));
            break;
        }
        case EnemyShape::star: {
            star(center, size * 0.66f, 5, 0.48f, degrees(-90.0f));
            break;
        }
        case EnemyShape::cross: {
            cross(center, size * 0.6f, 0.36f);
            break;
        }
    }
}
