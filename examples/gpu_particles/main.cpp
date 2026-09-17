#include <p5cpp/p5cpp.hpp>

#include <cstdint>
#include <cstring>
#include <vector>

using namespace p5;

namespace
{
    constexpr uint32_t windowWidth = 800;
    constexpr uint32_t windowHeight = 600;
    constexpr uint32_t particleCount = 4000;
    constexpr uint32_t workgroupSize = 64;

    struct Particle
    {
        float posX, posY;
        float velX, velY;
    };
} // namespace

struct ComputeParticlesSketch : Sketch
{
    std::optional<StorageBuffer> particleBuffer;
    std::optional<StorageBuffer> paramsBuffer;
    std::optional<ComputeShader> computeShader;
    std::vector<Particle> particles;

    void setup() override
    {
        setWindowSize(windowWidth, windowHeight);

        particles.resize(particleCount);
        for (Particle& particle : particles) {
            particle.posX = random(0.0f, static_cast<float>(windowWidth));
            particle.posY = random(0.0f, static_cast<float>(windowHeight));
            particle.velX = random(-120.0f, 120.0f);
            particle.velY = random(-120.0f, 120.0f);
        }

        std::vector<uint8_t> particleBytes(particles.size() * sizeof(Particle));
        std::memcpy(particleBytes.data(), particles.data(), particleBytes.size());
        particleBuffer = createStorageBuffer(particleBytes.size(), particleBytes);

        const float initialDeltaTime = 0.0f;
        paramsBuffer = createStorageBuffer(sizeof(float), std::span {reinterpret_cast<const uint8_t*>(&initialDeltaTime), sizeof(float)});

        computeShader = loadComputeShaderFromMemory(R"(
            struct Particle {
                pos: vec2f,
                vel: vec2f,
            };

            @group(0) @binding(0) var<storage, read_write> particles: array<Particle>;
            @group(0) @binding(1) var<storage, read> params: array<f32>;

            const width: f32 = 800.0;
            const height: f32 = 600.0;

            @compute @workgroup_size(64)
            fn main(@builtin(global_invocation_id) id: vec3u) {
                if (id.x >= arrayLength(&particles)) {
                    return;
                }

                var particle = particles[id.x];
                let deltaTime = params[0];

                particle.pos = particle.pos + particle.vel * deltaTime;

                if (particle.pos.x < 0.0 || particle.pos.x > width) {
                    particle.vel.x = -particle.vel.x;
                    particle.pos.x = clamp(particle.pos.x, 0.0, width);
                }
                if (particle.pos.y < 0.0 || particle.pos.y > height) {
                    particle.vel.y = -particle.vel.y;
                    particle.pos.y = clamp(particle.pos.y, 0.0, height);
                }

                particles[id.x] = particle;
            }
        )");
    }

    void draw() override
    {
        background(rgba(12, 12, 20));

        if (not particleBuffer.has_value() or not paramsBuffer.has_value() or not computeShader.has_value()) {
            return;
        }

        const float deltaTime = static_cast<float>(getDeltaTime());
        paramsBuffer->updateData(
            std::span {
                reinterpret_cast<const uint8_t*>(&deltaTime),
                sizeof(float),
            }
        );

        const uint32_t groupCount = (particleCount + workgroupSize - 1) / workgroupSize;
        dispatchCompute(
            *computeShader,
            {
                {"particles", *particleBuffer},
                {"params", *paramsBuffer},
            },
            groupCount
        );

        const std::vector<uint8_t> resultBytes = particleBuffer->readData();
        particles.resize(particleCount);
        std::memcpy(particles.data(), resultBytes.data(), resultBytes.size());

        noStroke();
        fill(rgba(120, 200, 255, 200));
        for (const Particle& particle : particles) {
            circle(particle.posX, particle.posY, 2.0f);
        }
    }
};

SketchSpec p5::createSpec()
{
    return {
        .sketch = [] {
            return std::make_unique<ComputeParticlesSketch>();
        }
    };
}
