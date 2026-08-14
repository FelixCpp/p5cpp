#include <p5cpp/p5cpp.hpp>

#include <cstdio>

using namespace p5;

// A small, reusable, sketch-agnostic plugin: updates the window title with the
// current FPS every half second. Registered by CustomPluginSketch::plugins() below,
// it runs after the built-in plugins (Window/Graphics are already set up) and wraps
// the sketch's own draw() — its post-next() code runs after CustomPluginSketch::draw().
struct FpsTitlePlugin : public Plugin
{
    void draw(Context& context, const Next& next) override
    {
        next();

        m_accumulatedTime += getDeltaTime();
        m_frameCount++;

        if (m_accumulatedTime >= 0.5) {
            char title[64];
            std::snprintf(title, sizeof(title), "custom_plugin example - %.1f fps", m_frameCount / m_accumulatedTime);
            setWindowTitle(title);

            m_accumulatedTime = 0.0;
            m_frameCount = 0;
        }
    }

private:
    double m_accumulatedTime = 0.0;
    int m_frameCount = 0;
};

struct CustomPluginSketch : public Sketch
{
    void setup() override
    {
        setWindowSize(640, 480);
        setWindowTitle("custom_plugin example");
    }

    void draw() override
    {
        background(0);

        fill(rgba(255, 100, 100));
        noStroke();

        const uint2 size = getWindowSize();
        ellipse(static_cast<float>(size.x) / 2.0f, static_cast<float>(size.y) / 2.0f, 50, 50);
    }

    std::vector<std::unique_ptr<Plugin>> plugins() override
    {
        std::vector<std::unique_ptr<Plugin>> result;
        result.push_back(std::make_unique<FpsTitlePlugin>());
        return result;
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<CustomPluginSketch>();
}
