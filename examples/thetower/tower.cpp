#include "tower.hpp"

using namespace p5;

void tower_show(const Tower& tower, const Meters& range)
{
    with([&tower, &range] {
        stroke(rgba(60, 180, 220));
        strokeWeight(3.0f);

        strokeJoin(StrokeJoin::round);
        beginShape(ShapeMode::path);
        for (size_t i = 0; i < 6; ++i) {
            const float t = i / 6.0f * TAU;
            const float x = tower.position.x + std::cos(t) * tower.radius;
            const float y = tower.position.y + std::sin(t) * tower.radius;

            vertex(x, y);
        }
        endShape(true);

        noFill();
        stroke(rgba(71.0f));
        strokeWeight(4.0f);
        circle(tower.position.x, tower.position.y, range.toPixels());
    });
}
