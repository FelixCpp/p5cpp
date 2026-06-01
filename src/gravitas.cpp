#include "p5.hpp"

#include <algorithm>
#include <vector>
#include <numbers>

using namespace p5;

struct BlackHole
{
    float2 position;
    float2 velocity;
    float2 acceleration;
    float2 noiseOffset;

    float animationTime;

    inline static constexpr float MAX_FORCE = 18.0f;
    inline static constexpr float MAX_SPEED = 40.0f;
    inline static constexpr float DAMPING = 0.985f;
    inline static constexpr float NOISE_SCALE = 0.35f;
    inline static constexpr float NOISE_SPEED = 0.12f;

    explicit BlackHole(float2 position)
        : position {position},
          velocity {0, 0},
          acceleration {0, 0},
          noiseOffset {random(500.0f), random(500.0f)},
          animationTime {0}
    {
    }

    void update()
    {
        animationTime += deltaTime;

        const float tx = noiseOffset.x + animationTime * NOISE_SPEED;
        const float ty = noiseOffset.y + animationTime * NOISE_SPEED;
        const float fx = (noise(tx, ty + 31.7f) * 2.0f - 1.0f) * MAX_FORCE;
        const float fy = (noise(tx + 53.3f, ty) * 2.0f - 1.0f) * MAX_FORCE;
        acceleration = {fx, fy};

        velocity += acceleration;
        velocity *= DAMPING;
        velocity = limit(velocity, MAX_SPEED);

        position += velocity * deltaTime;
        acceleration = {0, 0};
    }

    void show()
    {
        pushState();
        translate(position.x, position.y);

        noFill();
        strokeCap(StrokeCap::butt);
        strokeJoin(StrokeJoin::miter);
        constexpr float maxRadius = 80.0f;
        constexpr float holeRadius = 12.0f;
        constexpr float twoPi = std::numbers::pi_v<float> * 2.0f;

        const color_t primaryColor = rgba(120, 40, 150);

        blendMode(BlendMode::additive);
        noStroke();
        beginShape();
        fill(primaryColor);
        vertex(0.0f, 0.0f);
        for (size_t s = 0; s <= 32; ++s) {
            const float angle = static_cast<float>(s) / 32.0f * twoPi;
            const float innerX = std::cos(angle) * holeRadius;
            const float innerY = std::sin(angle) * holeRadius;
            const float outerX = std::cos(angle) * maxRadius;
            const float outerY = std::sin(angle) * maxRadius;
            fill(primaryColor);
            vertex(innerX, innerY);
            fill(withAlpha(primaryColor, 0));
            vertex(outerX, outerY);
        }
        endShape(ShapeType::triangleStrip, false);

        constexpr int spirals = 3;
        constexpr float spiralTurns = 2.5f;
        constexpr int steps = 300;

        stroke(primaryColor);
        strokeWeight(1.6f);

        for (int i = 0; i < spirals; ++i) {
            const float armOffset = twoPi / static_cast<float>(spirals) * static_cast<float>(i);

            beginShape();
            for (int s = 0; s < steps; ++s) {
                const float pct = static_cast<float>(s) / static_cast<float>(steps);
                const float angle = pct * spiralTurns * twoPi + armOffset + animationTime;

                const float r = holeRadius * std::pow(maxRadius / holeRadius, pct);
                const float x = std::cos(angle) * r;
                const float y = std::sin(angle) * r;
                curveVertex(x, y);
            }
            endShape(ShapeType::polygon, false);
        }

        for (int ring = 1; ring <= 5; ++ring) {
            const float radius = holeRadius + ring * (maxRadius - holeRadius) / 6.0f;
            const float alpha = remap(ring, 1, 5, 170, 10);
            stroke(180, 120, 255, static_cast<int>(alpha));
            strokeWeight(ring == 1 ? 2.5f : 1.0f);
            circle(0.0f, 0.0f, radius * 2);
        }

        blendMode(BlendMode::alpha);
        stroke(primaryColor);
        strokeWeight(2.0f);
        fill(0);
        circle(0.0f, 0.0f, holeRadius * 2);

        popState();
    }
};

struct Gravitas : Sketch
{
    BlackHole hole = BlackHole {float2 {300.0f, 300.0f}};

    void setup() override
    {
        createCanvas(1280, 720);
    }

    void draw() override
    {
        background(51);
        hole.update();
        hole.show();
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<Gravitas>();
}
