#include "arena.hpp"

#include <cmath>

#include "palette.hpp"

using namespace p5;

namespace
{
    constexpr float cell = 40.0f;

    // a stack of circles, each one adding a little light, stands in for a radial gradient
    void show_floor_glow(const float2& center, const float radius)
    {
        constexpr int rings = 30;

        blendMode(BlendMode::additive);
        noStroke();
        fill(withOpacity(floorGlowColor, 0.022f));

        for (int i = rings; i >= 1; --i) {
            circle(center.x, center.y, radius * static_cast<float>(i) / rings);
        }
    }

    // the lines keep their world positions, the view only decides how many of them are needed
    void show_grid(const rect2f& view)
    {
        const float right = view.left + view.width;
        const float bottom = view.top + view.height;

        stroke(withOpacity(gridColor, 0.055f));
        strokeWeight(1.0f);

        for (float x = std::ceil(view.left / cell) * cell; x < right; x += cell) {
            line(x, view.top, x, bottom);
        }
        for (float y = std::ceil(view.top / cell) * cell; y < bottom; y += cell) {
            line(view.left, y, right, y);
        }
    }

    void show_spawn_ring(const float2& center, const float spawnDistance)
    {
        noFill();
        stroke(withOpacity(gridColor, 0.22f));
        strokeWeight(1.5f);
        strokeDashPattern({9.0f, 15.0f});
        strokeDashOffset(static_cast<float>(getGlobalTime()) * 6.0f);
        circle(center.x, center.y, spawnDistance);
    }
}

void arena_backdrop_show(const p5::rect2f& view, const p5::float2& center, const float glowRadius)
{
    with([&] {
        noStroke();
        fill(floorColor);
        rect(view.left, view.top, view.width, view.height);

        show_grid(view);
        show_floor_glow(center, glowRadius);
    });
}

void arena_show(const p5::rect2f& view, const p5::float2& center, const float spawnDistance)
{
    arena_backdrop_show(view, center, spawnDistance * 1.2f);

    with([&] {
        show_spawn_ring(center, spawnDistance);
    });
}
