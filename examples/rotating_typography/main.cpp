#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

using namespace p5;
using namespace p5::animation;

struct Type
{
    std::string sentence;

    float angle;
    float dir;
    float amt;
    float min;
    float angleInc;
    float size;
    float x;
    float y;
    uint8_t alpha;

    explicit Type(const std::string& sentence, const float radius, const float dir, const float angleInc, const uint8_t alpha, const float size)
        : sentence {sentence},
          angle {0.0f},
          dir {dir},
          amt {0.0f},
          min {0.0f},
          angleInc {angleInc},
          size {size},
          x {radius * std::cos(angle)},
          y {radius * std::sin(angle)},
          alpha {alpha}
    {
    }

    void update(float deltaTime)
    {
        angle = min + easeInQuad(amt) * angleInc;

        if (amt > 1.0f) {
            amt = 0.0f;
            min += angleInc;
        } else {
            amt += deltaTime * 0.5f;
        }
    }

    void display()
    {
        textSize(size);
        noStroke();
        fill(rgba(255, alpha));
        withMatrix([&] {
            rotate(radians(angle * dir));

            for (size_t i = 0; i < sentence.length(); ++i) {
                const char character = sentence.at(i);

                withMatrix([&] {
                    const float letterAngle = 360.0f / static_cast<float>(sentence.length()) * static_cast<float>(i);
                    rotate(radians(letterAngle));
                    text(std::format("{}", character), x, y);
                });
            }
        });
    }
};

struct RotatingTypography : Sketch
{
    inline static constexpr size_t num = 8;
    std::vector<Type> texts;
    std::shared_ptr<Font> font = loadFontFromFile("fonts/Lexend_Deca/static/LexendDeca-Bold.ttf");

    void setup() override
    {
        setWindowSize(400, 400);

        std::string sentence = "HELLO";
        for (size_t i = 0; i < num; ++i) {
            sentence += "HELLO";
            const float radius = 30.0f * static_cast<float>(i + 1);
            const float dir = (i % 2 == 0) ? 1.0f : -1.0f;
            const float angleInc = 90.0f / static_cast<float>(i + 1);
            const uint8_t alpha = 255 - static_cast<uint8_t>(255.0f / static_cast<float>(num) * static_cast<float>(i));
            const float size = static_cast<float>(i + 1) * 6.0f;
            texts.emplace_back(sentence, radius, dir, angleInc, alpha, size);
        }
    }

    void draw() override
    {
        for (Type& text : texts) {
            text.update(getDeltaTime());
        }

        textFont(font);
        background(rgba(0, 0, 100));
        translate(getWidth() * 0.5f, getHeight() * 0.5f);

        for (Type& text : texts) {
            text.display();
        }
    }
};

SketchSpec p5::createSpec()
{
    return {
        .sketch = [] {
            return std::make_unique<RotatingTypography>();
        }
    };
}
