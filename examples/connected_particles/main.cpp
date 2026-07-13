#include <p5cpp/p5cpp.hpp>

using namespace p5cpp;

namespace
{
    struct Colorizer
    {
        virtual ~Colorizer() = default;
        virtual void update(float deltaTime) = 0;
        virtual color_t getColor(float2 position, float2 velocity) = 0;
    };

    struct HueColorizer : Colorizer
    {
        void update(float deltaTime) override
        {
            // No update needed for this colorizer
        }

        color_t getColor(float2 position, float2 velocity) override
        {
            const float hue = degrees(std::atan2(velocity.y, velocity.x)) + 180.0f;
            return hsv(hue, 1.0f, 1.0f, 255);
        }
    };

    struct GlobalHueColorizer : Colorizer
    {
        float hue;

        GlobalHueColorizer() : hue(0.0f) {}

        void update(float deltaTime) override
        {
            hue += 30.0f * deltaTime; // Adjust the speed of hue change as needed
            if (hue > 360.0f) {
                hue -= 360.0f;
            }
        }

        color_t getColor(float2 position, float2 velocity) override
        {
            return hsv(hue, 1.0f, 1.0f, 255);
        }
    };

    struct PositionBasedColorizer : Colorizer
    {
        void update(float deltaTime) override
        {
            // No update needed for this colorizer
        }

        color_t getColor(float2 position, float2 velocity) override
        {
            const float hue = std::fmod(position.x + position.y, 360.0f);
            return hsv(hue, 1.0f, 1.0f, 255);
        }
    };

    inline static std::unique_ptr<Colorizer> s_colorizer;

    struct Particle
    {
        float2 position;
        float2 velocity;
        color_t color;
        float lifetimeInSeconds;
        float initialLifetimeInSeconds;

        explicit Particle(const float2& position, const float2& velocity, float lifetimeInSeconds)
            : position(position), velocity(velocity), color(s_colorizer->getColor(position, velocity)), lifetimeInSeconds(lifetimeInSeconds), initialLifetimeInSeconds(lifetimeInSeconds)
        {
        }

        bool isAlive() const
        {
            return lifetimeInSeconds > 0.0f;
        }

        float getAlpha() const
        {
            return std::max(lifetimeInSeconds / initialLifetimeInSeconds, 0.0f);
        }

        void update(float deltaTime)
        {
            lifetimeInSeconds -= deltaTime;
            position += velocity * deltaTime;
        }

        void show() const
        {
            const color_t fillColor = withAlpha(color, static_cast<int>(getAlpha() * 255.0f));

            noStroke();
            fill(fillColor);
            ellipse(position.x, position.y, 12.0f, 12.0f);
        }
    };

    struct ParticleSystem
    {
        std::vector<Particle> particles;

        explicit ParticleSystem(size_t particleCount)
        {

            spawnParticles(
                []() {
                    const auto width = getPhysicalWidth();
                    const auto height = getPhysicalHeight();
                    const float positionX = randomFloat(static_cast<float>(width));
                    const float positionY = randomFloat(static_cast<float>(height));
                    return float2 {positionX, positionY};
                },
                particleCount
            );
        }

        void spawnParticles(float2 (*getPosition)(), size_t count)
        {
            particles.reserve(particles.size() + count);
            for (size_t i = 0; i < count; ++i) {
                const float2 randomDirection = float2::randomUnit();
                const float2 velocity = randomDirection * randomFloat(50.0f, 150.0f);
                const float lifetimeInSeconds = randomFloat(1.0f, 5.0f);

                particles.emplace_back(getPosition(), velocity, lifetimeInSeconds);
            }
        }

        void update(float deltaTime)
        {
            for (Particle& particle : particles) {
                particle.update(deltaTime);

                if (isOutOfSight(particle.position)) {
                    const auto [width, height] = getCanvasSize();
                    particle.position.x = std::clamp(particle.position.x, 0.0f, static_cast<float>(width));
                    particle.position.y = std::clamp(particle.position.y, 0.0f, static_cast<float>(height));
                    particle.velocity = float2::randomUnit() * randomFloat(50.0f, 150.0f);
                }
            }

            for (size_t i = 0; i < particles.size(); ++i) {
                const Particle& particle = particles[i];
                for (size_t j = i + 1; j < particles.size(); ++j) {
                    const Particle& otherParticle = particles[j];

                    const float distance = (particle.position - otherParticle.position).length();
                    if (distance > 100.0f) continue;

                    const float alpha = 1.0f - (distance / 100.0f);
                    const float particleAlpha = (particle.getAlpha() * alpha) * 255.0f;
                    const float otherParticleAlpha = (otherParticle.getAlpha() * alpha) * 255.0f;

                    beginShape();
                    stroke(withAlpha(particle.color, particleAlpha));
                    vertex(particle.position.x, particle.position.y);
                    stroke(withAlpha(otherParticle.color, otherParticleAlpha));
                    vertex(otherParticle.position.x, otherParticle.position.y);
                    endShape(ShapeType::lines, false);
                }
            }

            std::erase_if(particles, [](const Particle& particle) {
                return not particle.isAlive();
            });
        }

        void show() const
        {
            for (const auto& particle : particles) {
                particle.show();
            }
        }

        bool isOutOfSight(const float2& position) const
        {
            const auto [width, height] = getCanvasSize();
            return position.x < 0.0f || position.x > static_cast<float>(width) || position.y < 0.0f || position.y > static_cast<float>(height);
        }
    };

    inline static constexpr const char* pixelateSource = R"(
        uniform float u_BlockSize;

        vec4 effect(vec2 uv, vec2 texelSize, sampler2D tex) {
            vec2 block = texelSize * max(u_BlockSize, 1.0);
            vec2 blockUV = (floor(uv / block) + 0.5) * block;
            return texture(tex, blockUV);
        }
    )";

    struct ConnectedParticleSketch : Sketch
    {
        std::unique_ptr<ParticleSystem> particleSystem;
        Shader pixelateShader;
        bool isMouseDown;

        void setup() override
        {
            setWindowSize(1280, 720);
            setWindowTitle("Connected Particles");
            frameRate(144.0f);

            pixelateShader = loadEffectShader(pixelateSource);

            // s_colorizer = std::make_unique<GlobalHueColorizer>();
            s_colorizer = std::make_unique<PositionBasedColorizer>();
            particleSystem = std::make_unique<ParticleSystem>(10);
            isMouseDown = false;
        }

        void event(const WindowEvent& event) override
        {
            if (event.type == EventType::mousePress and event.mouseButton.button == MouseButton::left) {
                isMouseDown = true;
            } else if (event.type == EventType::mouseRelease and event.mouseButton.button == MouseButton::left) {
                isMouseDown = false;
            }
        }

        void draw() override
        {
            s_colorizer->update(getDeltaTime());

            background(21, 75);

            const int dragThreshold = 2;
            const int mouseDragX = getMouseX() - getPMouseX();
            const int mouseDragY = getMouseY() - getPMouseY();
            const bool isMouseDragging = (std::abs(mouseDragX) > dragThreshold) || (std::abs(mouseDragY) > dragThreshold);

            if (isMouseDown and isMouseDragging) {
                const int newParticlesCount = std::sqrt(mouseDragX * mouseDragX + mouseDragY * mouseDragY) / 5;

                particleSystem->spawnParticles(
                    []() {
                        const int mx = getMouseX();
                        const int my = getMouseY();
                        return float2 {static_cast<float>(mx), static_cast<float>(my)};
                    },
                    newParticlesCount
                );
            }

            blendMode(BlendMode::additive);
            particleSystem->update(getDeltaTime());
            particleSystem->show();

            setUniform(pixelateShader, "u_BlockSize", uniform(3.0f));
            effect(pixelateShader);

            blendMode(BlendMode::alpha);
            fill(255);
            textSize(32.0f);
            text("Frame Rate: " + std::to_string(static_cast<int>(getFrameRate())), 20.0f, 40.0f);
        }
    };
} // namespace

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<ConnectedParticleSketch>();
    }
} // namespace p5cpp
