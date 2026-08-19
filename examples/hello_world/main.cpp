#include <p5cpp/p5cpp.hpp>
#include <p5cpp/p5cpp_animation.hpp>

using namespace p5;

struct Particle
{
    float2 position;
    float2 velocity;
    float2 acceleration;
    tween<float> size;
    float lifetime;
    float timeAlive;
    tween<color_t> color;
};

Particle particle_create(float2 position)
{
    Particle particle = {
        .position = position,
        .velocity = float2 {random(-1.0f, 1.0f), random(-1.0f, 1.0f)} * 100.0f,
        .acceleration = {0.0f, 0.0f},
        .size = createTween(0.0f, random(5.0f, 15.0f) * 0.5f, 1.0f, &easeLinear),
        .lifetime = random(1.0f, 2.0f), // random(0.1f, 0.5f),
        .timeAlive = 0.0f,
        .color = createTween(rgba(255), rgba(random(100, 255), random(100, 255), random(100, 255), 255), 1.0f),
    };

    restart(particle.size);
    restart(particle.color);

    return particle;
}

void particle_apply_force(Particle& particle, const float2& force)
{
    particle.acceleration = particle.acceleration + force;
}

void particle_update(Particle& particle, const float2& mouse_position, float deltaTime)
{
    float2 force = fixedLength(mouse_position - particle.position, 290.0f);
    particle_apply_force(particle, force);

    particle.velocity = particle.velocity + particle.acceleration * deltaTime;
    particle.velocity = particle.velocity * 0.98f; // Damping
    particle.position = particle.position + particle.velocity * deltaTime;
    particle.acceleration = {0.0f, 0.0f};

    particle.timeAlive = std::min(particle.timeAlive + deltaTime, particle.lifetime);

    advance(particle.size, deltaTime);
    advance(particle.color, deltaTime);
}

struct EffectsSketch : public Sketch
{
    std::unique_ptr<Particle[]> particles;
    size_t numParticles = 1000;

    void setup() override
    {
        setWindowSize(400, 400);

        particles = std::make_unique<Particle[]>(numParticles);
        for (size_t i = 0; i < numParticles; ++i) {
            particles[i] = particle_create(float2 {0.0f, 0.0f});
        }
    }

    void draw() override
    {
        background(rgba(0));

        const float2 mouse_position = {.x = static_cast<float>(getMouseX()), .y = static_cast<float>(getMouseY())};

        for (size_t i = 0; i < numParticles; ++i) {
            Particle& particle = particles[i];
            particle_update(particle, mouse_position, static_cast<float>(getDeltaTime()));

            float alpha = 1.0f - (particle.timeAlive / particle.lifetime);
            const color_t baseColor = value(particle.color);
            color_t color = rgba(getRed(baseColor), getGreen(baseColor), getBlue(baseColor), static_cast<int32_t>(alpha * 255));

            fill(color);
            noStroke();
            circle(particle.position.x, particle.position.y, value(particle.size));

            if (particle.timeAlive >= particle.lifetime) {
                particles[i] = particle_create(mouse_position);
            }
        }
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<EffectsSketch>();
}
