#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

using namespace p5;

struct Particle
{
    float2 position;
    color_t color;

    Particle(float x, float y, color_t color)
        : position {.x = x, .y = y}, color(color)
    {
    }

    void update(float deltaTime)
    {
    }

    void show()
    {
        fill(rgba(0));
        circle(position.x, position.y, 10.0f);
    }
};

struct InteractiveParticles : Sketch
{
    std::vector<Particle> particles;

    void setup() override
    {
        setWindowSize(400, 400);

        int imageWidth = 400;
        int imageHeight = 400;
        std::vector<color_t> imageData(imageWidth * imageHeight);
        for (int y = 0; y < imageHeight; ++y) {
            for (int x = 0; x < imageWidth; ++x) {
                imageData[y * imageWidth + x] = rgba(random(255), random(255), random(255));
            }
        }

        for (int y = 0; y < imageHeight; y += 5) {
            for (int x = 0; x < imageWidth; x += 5) {
                color_t pixelColor = imageData[y * imageWidth + x];
                particles.emplace_back(x, y, pixelColor);
            }
        }
    }

    void draw() override
    {
        background(rgba(40));

        for (size_t i = 0; i < particles.size(); ++i) {
            Particle& particle = particles[i];
            particle.update(static_cast<float>(getDeltaTime()));
            particle.show();
        }
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<InteractiveParticles>();
}
