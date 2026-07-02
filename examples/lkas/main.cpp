#include <p5cpp/p5cpp.hpp>

using namespace p5cpp;

struct Particle
{
    float2 pos;
    float2 vel;
    float life; // 0..1
};

class MyCustomPlugin : public Module
{
public:
    void setup(AppContext& context, Next next) override
    {
        blurCanvas = createFramebuffer(getWidth(), getHeight());

        next();
    }

    void event(AppContext& context, WindowEvent& event, Next next) override
    {
        next();
    }

    void draw(AppContext& context, Next next) override
    {
        pushCanvas(blurCanvas);
        next();
        popCanvas();
    }

    void destroy(AppContext& context, Next next) override
    {
        next();
    }

private:
    std::shared_ptr<Framebuffer> blurCanvas;
};

struct ParticleSketch : Sketch
{
    std::vector<Particle> particles;

    void setup() override
    {
        randomSeed(42);
        setWindowSize(800, 600);
        frameRate(60);
    }

    void draw() override
    {
        background(10, 10, 10, 30); // alpha trail

        fill(255);
        textSize(24.0f);
        textAlign(TextAlign::topLeft);
        text("Framerate: " + std::to_string((int)getFrameRate()), 10, 10);

        const float dt = getDeltaTime();

        // Spawn
        for (int i = 0; i < 5; ++i) {
            float2 dir = float2::randomUnit();
            particles.push_back({
                {(float)getWidth() / 2, (float)getHeight() / 2},
                dir * randomFloat(50.0f, 150.0f),
                1.0f,
            });
        }

        // Update & draw
        noStroke();
        for (auto& p : particles) {
            p.pos += p.vel * dt;
            p.life -= dt * 0.5f;
            p.life = std::max(p.life, 0.0f);

            int alpha = (int)(p.life * 255);
            fill(200, 120, 255, alpha);
            circle(p.pos.x, p.pos.y, 6);
        }

        // Remove dead
        std::erase_if(particles, [](const Particle& p) {
            return p.life <= 0.0f;
        });
    }
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<ParticleSketch>();
    }
} // namespace p5cpp
