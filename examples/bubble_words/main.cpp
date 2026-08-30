#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

using namespace p5;
using namespace p5::animation;

struct BubbleWord
{
    std::vector<float2> points;
    rect2f bounds;

    explicit BubbleWord(const std::string& text, Font* font)
    {
        const auto textPoints = textToPoints(
            text,
            0.0f,
            0.0f,
            TextToPointsOptions {
                .sampleFactor = 0.075f,
                .simplifyThreshold = 0.0f,
                .font = font,
                .size = 128.0f,
            }
        );

        for (const auto& pt : textPoints) {
            points.push_back(pt.position);
        }

        bounds = computeBounds(points);
        alignPointsAroundCenter(points, bounds.width, bounds.height);
    }

    void show(float alpha) const
    {
        fill(rgba(255, static_cast<int32_t>(alpha * 255.0f)));
        noStroke();
        for (const float2& point : points) {
            circle(point.x, point.y, 4.0f);
        }
    }

private:
    static void alignPointsAroundCenter(std::vector<float2>& points, float width, float height)
    {
        const float centerX = width * 0.5f;
        const float centerY = height * 0.5f;

        for (auto& point : points) {
            point.x -= centerX;
            point.y -= centerY;
        }
    }

    static rect2f computeBounds(const std::vector<float2>& points)
    {
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;
        for (const auto& pt : points) {
            left = std::min(left, pt.x);
            top = std::min(top, pt.y);
            right = std::max(right, pt.x);
            bottom = std::max(bottom, pt.y);
        }

        return {left, top, right - left, bottom - top};
    }
};

struct TextToPoints : Sketch
{
    static constexpr float holdDuration = 2.0f;
    static constexpr float fadeDuration = 0.6f;

    std::vector<BubbleWord> bubbleWords;

    size_t currentIndex = 0;
    size_t nextIndex = 1;

    tween<float> crossfade = createTween(0.0f, 1.0f, fadeDuration, easeInOutSine);

    static constexpr size_t fadeStep = 1;

    sequence cycle = createSequence({
        wait(holdDuration),
        play(crossfade),
        call([this] {
            currentIndex = nextIndex;
            nextIndex = (currentIndex + 1) % bubbleWords.size();
        }),
    });

    void setup() override
    {
        setWindowSize(400, 400);
        with([&] {
            const std::shared_ptr<Font> font = loadFontFromFile("fonts/Lexend_Deca/static/LexendDeca-Bold.ttf");
            bubbleWords.emplace_back("First", font.get());
            bubbleWords.emplace_back("Second", font.get());
            bubbleWords.emplace_back("Third", font.get());
        });

        loop(cycle, LoopMode::loop);
        restart(cycle);
    }

    void draw() override
    {
        background(rgba(31, 31, 51));
        translate(getWidth() * 0.5f, getHeight() * 0.5f);

        advance(cycle, static_cast<float>(getDeltaTime()));

        const float t = (currentStep(cycle) == fadeStep) ? value(crossfade) : 0.0f; // 0 = nur current sichtbar, 1 = nur next
        bubbleWords[currentIndex].show(1.0f - t);
        if (nextIndex != currentIndex) {
            bubbleWords[nextIndex].show(t);
        }
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
