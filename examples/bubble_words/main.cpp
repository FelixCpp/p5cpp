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
        // SOMEWHERE IS A BUG IN HERE
        noFill();
        stroke(rgba(255, 255, 255));
        strokeWeight(2.0f);
        for (const Contour& contour : contours) {
            beginShape();
            for (const float2& point : contour.points) {
                vertex(point.x, point.y);
            }
            endShape(true);
        }
    }

private:
    static std::vector<Contour> buildContoursFromTextPoints(const std::vector<TextPoint>& textPoints)
    {
        std::vector<Contour> contours;
        if (textPoints.empty()) {
            return contours;
        }

        Contour currentContour;
        currentContour.points.push_back(textPoints[0].position);

        for (size_t i = 1; i < textPoints.size(); ++i) {
            const float2& prevPoint = textPoints[i - 1].position;
            const float2& currPoint = textPoints[i].position;

            // If the distance between the current point and the previous point is greater than a threshold, start a new contour
            if (length(currPoint - prevPoint) > 5.0f) {
                contours.push_back(currentContour);
                currentContour.points.clear();
            }

            currentContour.points.push_back(currPoint);
        }

        // Add the last contour if it has points
        if (!currentContour.points.empty()) {
            contours.push_back(currentContour);
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
    sequential_transition_chain fades = sequential(
        repeat(
            [this] {
                return sequential(
                    sequential(
                        tween(fadeDuration, curves::easeInOutSine, [this](float progress) { alphas[0] = lerp(0.0f, 1.0f, progress); }),
                        wait_for(holdDuration),
                        tween(fadeDuration, curves::easeInOutSine, [this](float progress) { alphas[0] = lerp(1.0f, 0.0f, progress); })
                    ),
                    sequential(
                        tween(fadeDuration, curves::easeInOutSine, [this](float progress) {alphas[1] = lerp(0.0f, 1.0f, progress); }),
                        wait_for(holdDuration),
                        tween(fadeDuration, curves::easeInOutSine, [this](float progress) { alphas[1] = lerp(1.0f, 0.0f, progress);})
                    ),
                    sequential(
                        tween(fadeDuration, curves::easeInOutSine, [this](float progress) {alphas[2] = lerp(0.0f, 1.0f, progress); }),
                        wait_for(holdDuration),
                        tween(fadeDuration, curves::easeInOutSine, [this](float progress) { alphas[2] = lerp(1.0f, 0.0f, progress);})
                    )
                );
            },
            std::nullopt
        )
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
        background(rgba(31, 31, 51));
        translate(getWidth() * 0.5f, getHeight() * 0.5f);

        fades.advance(getDeltaTime());

        for (size_t i = 0; i < bubbleWords.size(); ++i) {
            bubbleWords[i].show(alphas[i]);
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
