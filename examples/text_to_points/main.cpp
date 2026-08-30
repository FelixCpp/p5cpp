#include <p5cpp/p5cpp.hpp>

using namespace p5;

// Demonstrates textToPoints(): sample points (with tangent angle) along a text string's glyph
// outlines. Move the mouse horizontally to change sampleFactor (point density) and vertically to
// change simplifyThreshold (how aggressively near-straight runs get pruned).
struct TextToPoints : Sketch
{
    std::shared_ptr<Font> font = loadFontFromFile("fonts/Lexend_Deca/static/LexendDeca-Bold.ttf");

    void setup() override
    {
        setWindowSize(900, 400);
    }

    void draw() override
    {
        background(rgba(10, 12, 20));

        const float sampleFactor = map(constrain(static_cast<float>(getMouseX()), 0.0f, getWidth()), 0.0f, getWidth(), 0.02f, 0.5f);
        const float simplifyThreshold = map(constrain(static_cast<float>(getMouseY()), 0.0f, getHeight()), 0.0f, getHeight(), 0.0f, radians(30.0f));

        textFont(font);
        textAlign(TextAlignment::center);
        // size (220px here) is passed via options.size rather than a prior textSize() call.
        const std::vector<TextPoint> points = textToPoints("p5cpp", getWidth() * 0.5f, getHeight() * 0.5f, {.sampleFactor = sampleFactor, .simplifyThreshold = simplifyThreshold, .size = 220.0f});

        noStroke();
        for (const TextPoint& point : points) {
            fill(rgba(255, 210, 90));
            circle(point.position.x, point.position.y, 4.0f);

            // A short tangent tick shows what `angle` (radians) describes at each sampled point.
            stroke(rgba(255, 210, 90, 90));
            strokeWeight(1.0f);
            line(point.position.x, point.position.y, point.position.x + std::cos(point.angle) * 10.0f, point.position.y + std::sin(point.angle) * 10.0f);
        }

        noStroke();
        fill(rgba(255));
        textAlign(TextAlignment::topLeft);
        textSize(16.0f);
        p5::text(std::format("points: {}   sampleFactor: {:.3f}   simplifyThreshold: {:.2f} rad", points.size(), sampleFactor, simplifyThreshold), 12.0f, 12.0f);
    }
};

SketchSpec p5::createSpec()
{
    return {
        .sketch = [] {
            return std::make_unique<TextToPoints>();
        }
    };
}
