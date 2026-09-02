#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

using namespace p5;
using namespace p5::animation;

struct Contour
{
    std::vector<float2> points;
};

struct BubbleWord
{
    std::vector<Contour> contours;
    rect2f bounds;

    explicit BubbleWord(const std::string& text, Font* font)
    {
        const std::vector<TextPoint> textPoints = textToPoints(
            text,
            0.0f,
            0.0f,
            TextToPointsOptions {
                .sampleFactor = 0.075f,
                .simplifyThreshold = 0.0f,
                // .font = font,
                .size = 128.0f,
            }
        );

        contours = buildContoursFromTextPoints(textPoints);
        bounds = computeBounds(contours);
        alignPointsAroundCenter(contours, bounds.width, bounds.height);
    }

    void show(float alpha) const
    {
        noFill();
        stroke(rgba(255, 255, 255, static_cast<int32_t>(alpha * 255.0f)));
        strokeWeight(2.0f);
        for (const Contour& contour : contours) {
            beginShape();
            for (const float2& point : contour.points) {
                const float offsetX = random(-1.0f, 1.0f) * 10.0f;
                const float offsetY = random(-1.0f, 1.0f) * 10.0f;
                vertex(point.x + offsetX, point.y + offsetY);
            }
            endShape(true);
        }
    }

private:
    static std::vector<Contour> buildContoursFromTextPoints(const std::vector<TextPoint>& textPoints)
    {
        std::vector<Contour> contours;

        std::optional<uint32_t> currentContourIndex;
        for (const TextPoint& textPoint : textPoints) {
            if (!currentContourIndex.has_value() || textPoint.contourIndex != *currentContourIndex) {
                contours.emplace_back();
                currentContourIndex = textPoint.contourIndex;
            }

            contours.back().points.push_back(textPoint.position);
        }

        return contours;
    }

    static void alignPointsAroundCenter(const std::span<Contour>& contours, float width, float height)
    {
        const float centerX = width * 0.5f;
        const float centerY = height * 0.5f;

        for (Contour& contour : contours) {
            for (float2& point : contour.points) {
                point.x -= centerX;
                point.y -= centerY;
            }
        }
    }

    static rect2f computeBounds(const std::span<Contour>& contours)
    {
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;

        for (const Contour& contour : contours) {
            for (const float2& pt : contour.points) {
                left = std::min(left, pt.x);
                top = std::min(top, pt.y);
                right = std::max(right, pt.x);
                bottom = std::max(bottom, pt.y);
            }
        }

        return {left, top, right - left, bottom - top};
    }
};

struct TextToPoints : Sketch
{
    static constexpr float holdDuration = 2.0f;
    static constexpr float fadeDuration = 0.6f;

    std::vector<BubbleWord> bubbleWords;
    std::vector<float> alphas {0.0f, 0.0f, 0.0f};

    // clang-format off
    RepeatingTransitionComposite fades = repeating(
        sequential({
            sequential({
                tween(fadeDuration, curves::easeInOutSine, [this](float progress) { alphas[0] = lerp(0.0f, 1.0f, progress); }),
                waitFor(holdDuration),
                tween(fadeDuration, curves::easeInOutSine, [this](float progress) { alphas[0] = lerp(1.0f, 0.0f, progress); })
            }),
            sequential({
                tween(fadeDuration, curves::easeInOutSine, [this](float progress) {alphas[1] = lerp(0.0f, 1.0f, progress); }),
                waitFor(holdDuration),
                tween(fadeDuration, curves::easeInOutSine, [this](float progress) { alphas[1] = lerp(1.0f, 0.0f, progress);})
            }),
            sequential({
                tween(fadeDuration, curves::easeInOutSine, [this](float progress) {alphas[2] = lerp(0.0f, 1.0f, progress); }),
                waitFor(holdDuration),
                tween(fadeDuration, curves::easeInOutSine, [this](float progress) { alphas[2] = lerp(1.0f, 0.0f, progress);})
            })
        })
    );

    // clang-format on

    void setup() override
    {
        setWindowSize(800, 400);
        with([&] {
            const std::shared_ptr<Font> font = loadFontFromFile("fonts/Lexend_Deca/static/LexendDeca-Bold.ttf");
            bubbleWords.emplace_back("First", font.get());
            bubbleWords.emplace_back("Second", font.get());
            bubbleWords.emplace_back("Third", font.get());
        });
    }

    void draw() override
    {
        background(rgba(31, 31, 51, 25));

        pushMatrix();
        translate(getWidth() * 0.5f, getHeight() * 0.5f);

        fades.advance(getDeltaTime());

        for (size_t i = 0; i < bubbleWords.size(); ++i) {
            bubbleWords[i].show(alphas[i]);
        }

        popMatrix();
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
