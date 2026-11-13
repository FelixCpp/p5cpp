#include "particle_burst.hpp"

#include <algorithm>
#include <cmath>

#include <p5cpp_animation/p5cpp_animation.hpp>

#include "palette.hpp"

using namespace p5;
using namespace p5::animation;

namespace
{
    // the counts scale with the enemy's area, so twice the size breaks into four times the pieces
    constexpr float areaPerShard = 110.0f;
    constexpr float areaPerSpark = 30.0f;

    constexpr int minShards = 4;
    constexpr int maxShards = 26;
    constexpr int minSparks = 8;
    constexpr int maxSparks = 70;

    constexpr float shardDrag = 3.0f;
    constexpr float sparkDrag = 9.0f;

    // the flash sits on the first frames, the shockwave runs a little longer
    constexpr float flashDuration = 0.13f;
    constexpr float waveDuration = 0.3f;

    int count_for(const float enemySize, const float areaPer, const int minCount, const int maxCount)
    {
        return std::clamp(static_cast<int>(std::round(enemySize * enemySize / areaPer)), minCount, maxCount);
    }

    void show_flash(const ParticleBurst& burst)
    {
        const float t = burst.age / flashDuration;
        if (t >= 1.0f) {
            return;
        }

        noStroke();
        fill(withOpacity(lerpColor(burst.color, rgba(255), 0.75f), 1.0f - t));
        circle(burst.position.x, burst.position.y, burst.size * (0.55f + 0.85f * t));
    }

    // one ring runs out of the body and thins out while it grows
    void show_shockwave(const ParticleBurst& burst)
    {
        const float t = burst.age / waveDuration;
        if (t >= 1.0f) {
            return;
        }

        const float grown = curves::easeOutCubic(t);

        noFill();
        stroke(withOpacity(burst.color, 0.8f * (1.0f - t) * (1.0f - t)));
        strokeWeight(0.5f + 6.0f * (1.0f - t));
        circle(burst.position.x, burst.position.y, burst.size * (0.45f + 1.25f * grown));
    }

    void show_sparks(const ParticleBurst& burst)
    {
        strokeCap(StrokeCap::round);

        for (const Spark& spark : burst.sparks) {
            const float alpha = 1.0f - curves::easeInQuad(spark.age / spark.lifetime);
            // the streak points back along the flight, so a fast spark draws a longer one
            const float2 tail = spark.position - spark.velocity * 0.05f;

            stroke(withOpacity(lerpColor(burst.color, rgba(255), 0.45f), alpha));
            strokeWeight(2.5f);
            line(tail.x, tail.y, spark.position.x, spark.position.y);
        }
    }

    void show_shards(const ParticleBurst& burst)
    {
        for (const Shard& shard : burst.shards) {
            const float t = shard.age / shard.lifetime;
            const float alpha = 1.0f - curves::easeInQuad(t);
            const float size = shard.size * (1.0f - 0.4f * t);

            with([&] {
                translate(shard.position.x, shard.position.y);
                rotate(shard.angle);

                fill(withOpacity(lerpColor(rgba(20, 22, 28), burst.color, 0.35f), alpha));
                stroke(withOpacity(burst.color, alpha));
                strokeWeight(2.0f);
                rect(-size * 0.5f, -size * 0.5f, size, size);
            });
        }
    }
} // namespace

ParticleBurst particle_burst_create(const ParticleDeathAnimation& animation, const p5::float2& position, const float enemySize)
{
    ParticleBurst burst = {
        .color = animation.color,
        .position = position,
        .size = enemySize,
        .age = 0.0f,
        .shards = {},
        .sparks = {},
    };

    const int shardCount = count_for(enemySize, areaPerShard, minShards, maxShards);
    const int sparkCount = count_for(enemySize, areaPerSpark, minSparks, maxSparks);

    burst.shards.reserve(shardCount);
    burst.sparks.reserve(sparkCount);

    for (int i = 0; i < shardCount; ++i) {
        burst.shards.push_back(Shard {
            .position = position + random2D(random(0.0f, enemySize * 0.35f)),
            .velocity = random2D(random(80.0f, 280.0f)),
            .size = random(enemySize * 0.18f, enemySize * 0.4f),
            .angle = radians(random(TAU)),
            .spin = radians(random(-9.0f, 9.0f)),
            .lifetime = random(0.45f, 0.95f),
            .age = 0.0f,
        });
    }

    for (int i = 0; i < sparkCount; ++i) {
        burst.sparks.push_back(Spark {
            .position = position + random2D(random(0.0f, enemySize * 0.2f)),
            .velocity = random2D(random(220.0f, 620.0f)),
            .lifetime = random(0.18f, 0.45f),
            .age = 0.0f,
        });
    }

    return burst;
}

void particle_burst_update(ParticleBurst& burst, const float deltaTime)
{
    burst.age += deltaTime;

    for (Shard& shard : burst.shards) {
        shard.age += deltaTime;
        shard.velocity = damp(shard.velocity, float2 {}, shardDrag, deltaTime);
        shard.position += shard.velocity * deltaTime;
        shard.angle += shard.spin * deltaTime;
    }

    for (Spark& spark : burst.sparks) {
        spark.age += deltaTime;
        spark.velocity = damp(spark.velocity, float2 {}, sparkDrag, deltaTime);
        spark.position += spark.velocity * deltaTime;
    }

    std::erase_if(burst.shards, [](const Shard& shard) { return shard.age >= shard.lifetime; });
    std::erase_if(burst.sparks, [](const Spark& spark) { return spark.age >= spark.lifetime; });
}

bool particle_burst_is_finished(const ParticleBurst& burst)
{
    return burst.shards.empty() && burst.sparks.empty() && burst.age >= waveDuration;
}

void particle_burst_show(const ParticleBurst& burst)
{
    with([&] {
        // the bright parts brighten each other where they overlap
        blendMode(BlendMode::additive);
        show_flash(burst);
        show_shockwave(burst);
        show_sparks(burst);
    });

    with([&] { show_shards(burst); });
}
