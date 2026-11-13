#include "hit_spark.hpp"

#include <algorithm>

#include <p5cpp_animation/p5cpp_animation.hpp>

#include "palette.hpp"

using namespace p5;
using namespace p5::animation;

namespace
{
    constexpr float lifetime = 0.22f;
    constexpr int ticks = 5;
}

void hit_sparks_spawn(std::vector<HitSpark>& sparks, const GameEvents& events)
{
    for (const DamageEvent& damage : events.damage) {
        // a kill gets the whole death burst, the flare would be lost in it
        if (damage.killed) {
            continue;
        }

        sparks.push_back(HitSpark {
            .position = damage.position,
            .color = damage.isCrit ? critColor : accentColor,
            .rotation = radians(random(TAU)),
            .size = damage.isCrit ? 20.0f : 13.0f,
            .lifetime = lifetime,
            .age = 0.0f,
        });
    }
}

void hit_sparks_update(std::vector<HitSpark>& sparks, const float deltaTime)
{
    for (HitSpark& spark : sparks) {
        spark.age += deltaTime;
    }

    std::erase_if(sparks, [](const HitSpark& spark) {
        return spark.age >= spark.lifetime;
    });
}

void hit_sparks_show(const std::vector<HitSpark>& sparks)
{
    with([&] {
        blendMode(BlendMode::additive);
        strokeCap(StrokeCap::round);
        noFill();

        for (const HitSpark& spark : sparks) {
            const float t = spark.age / spark.lifetime;
            const float alpha = 1.0f - curves::easeInQuad(t);
            const float grown = curves::easeOutCubic(t);

            stroke(withOpacity(spark.color, 0.7f * alpha));
            strokeWeight(1.5f);
            circle(spark.position.x, spark.position.y, spark.size * (0.2f + 0.8f * grown));

            // a handful of ticks shoot out of the ring, they make the hit read as an impact
            stroke(withOpacity(lerpColor(spark.color, rgba(255), 0.5f), alpha));
            strokeWeight(2.0f);
            for (int i = 0; i < ticks; ++i) {
                const float2 direction = fromAngle(spark.rotation + degrees(360.0f / ticks * i));
                const float2 from = spark.position + direction * (spark.size * (0.3f + 0.7f * grown));
                const float2 to = from + direction * (spark.size * 0.35f * (1.0f - t));
                line(from.x, from.y, to.x, to.y);
            }
        }
    });
}
