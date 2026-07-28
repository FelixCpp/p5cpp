#include "p5cpp/graphics/font.hpp"
#include <p5cpp/p5cpp.hpp>

using namespace p5cpp;

#include <chrono>
#include <limits>

struct LocalTime
{
    int hour;
    int minute;
    int second;

    inline constexpr bool operator==(const LocalTime& other) const = default;
    inline constexpr bool operator!=(const LocalTime& other) const = default;
};

LocalTime getLocalTime()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm = *std::localtime(&now_c);

    return LocalTime {
        .hour = local_tm.tm_hour,
        .minute = local_tm.tm_min,
        .second = local_tm.tm_sec
    };
}

std::string formatTime(const LocalTime& time)
{
    char buffer[9]; // HH:MM:SS
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", time.hour, time.minute, time.second);
    return std::string(buffer);
}

struct TextPoint
{
    float2 targetPosition;
    float2 currentPosition;
    float2 velocity;
    float stiffness = 400.0f; // higher = snaps to target faster
    float damping = 28.0f;    // lower = more overshoot/bounce, higher = smoother stop (critical ~= 2*sqrt(stiffness))

    void update(float deltaTime)
    {
        const float2 displacement = targetPosition - currentPosition;
        const float2 acceleration = displacement * stiffness - velocity * damping;
        velocity += acceleration * deltaTime;
        currentPosition += velocity * deltaTime;
    }
};

float2 contourCentroid(const TextContour& contour)
{
    float2 sum {0.0f, 0.0f};
    for (const float2& point : contour) {
        sum += point;
    }
    return contour.empty() ? sum : sum / static_cast<float>(contour.size());
}

struct TextContourPoints
{
    static constexpr size_t maxPointsPerContour = 200;

    std::vector<TextPoint> points;
    size_t activeCount = 0;
    float2 centroid;
    bool active = true;

    explicit TextContourPoints(const TextContour& contour)
    {
        retarget(contour);
    }

    // Reassigns as many points as the contour has vertices (capped at
    // maxPointsPerContour, evenly subsampled if there are more). Because we
    // never sample more points than there are distinct vertices, every point
    // maps to its own vertex - no duplicate positions to filter out.
    void retarget(const TextContour& contour)
    {
        centroid = contourCentroid(contour);
        activeCount = std::min(maxPointsPerContour, contour.size());

        for (size_t i = 0; i < activeCount; ++i) {
            const size_t vertexIndex = static_cast<size_t>(remap(i, 0, activeCount - 1, 0, contour.size() - 1));

            if (i < points.size()) {
                points[i].targetPosition = contour[vertexIndex];
            } else {
                points.push_back(TextPoint {
                    .targetPosition = contour[vertexIndex],
                    .currentPosition = contour[vertexIndex],
                });
            }
        }

        active = true;
    }

    // No contour matched this slot this frame. Points are left exactly where
    // they are (not cleared/moved) so that if this slot is matched again
    // later, the spring physics can smoothly animate from wherever they
    // already are instead of popping in fresh.
    void deactivate()
    {
        active = false;
    }

    void update(float deltaTime)
    {
        if (not active) {
            return;
        }

        for (size_t i = 0; i < activeCount; ++i) {
            points[i].update(deltaTime);
        }
    }

    void show() const
    {
        if (not active) {
            return;
        }

        noFill();
        stroke(255);
        strokeWeight(2.0f);
        beginShape();
        for (size_t i = 0; i < activeCount; ++i) {
            const float2& p = points[i].currentPosition;
            vertex(p.x, p.y);
            // ellipse(p.x, p.y, 2.0f, 2.0f);
        }
        endShape(ShapeType::polygon, true);
    }
};

struct InteractiveClock : Sketch
{
    Font font;
    std::vector<TextContourPoints> contourPoints;

    void setup() override
    {
        setWindowSize(800, 600);
        font = loadFont("example_assets/MapleMono-NF-Regular.ttf");
    }

    void draw() override
    {
        const LocalTime now = getLocalTime();
        std::string timeString = formatTime(now);

        const auto [canvasWidth, canvasHeight] = static_cast<float2>(getCanvasSize());
        const float centerX = canvasWidth / 2.0f;
        const float centerY = canvasHeight / 2.0f;

        textSize(128.0f);
        textAlign(TextAlign::center);
        textFont(font);
        textToPointsSpacing(20.f);
        textToPointsDetail(12);
        const auto layout = textLayout(timeString, centerX, centerY);
        auto contours = textToPoints(timeString, 0.0f, 0.0f);

        for (auto& contour : contours) {
            for (auto& point : contour) {
                const float randomOffsetX = randomFloat(-1.0f, 1.0f);
                const float randomOffsetY = randomFloat(-1.0f, 1.0f);
                const float randomScale = randomFloat(0.0f, 5.0f);
                point.x += randomOffsetX * randomScale;
                point.y += randomOffsetY * randomScale;
            }
        }

        // Match this frame's contours to existing slots by nearest centroid,
        // so a shape keeps its point cloud's identity across frames even
        // when unrelated contours elsewhere in the text appear/disappear
        // (e.g. a digit gaining or losing a hole shifts contour order).
        // Slots left unclaimed are deactivated rather than destroyed, so
        // they can be smoothly reused if their shape reappears later.
        std::vector<bool> claimed(contourPoints.size(), false);

        for (const TextContour& contour : contours) {
            const float2 contourCenter = contourCentroid(contour);

            size_t bestSlot = contourPoints.size();
            float bestDistanceSq = std::numeric_limits<float>::max();

            for (size_t i = 0; i < contourPoints.size(); ++i) {
                if (claimed[i]) {
                    continue;
                }

                const float distanceSq = (contourPoints[i].centroid - contourCenter).lengthSquared();
                if (distanceSq < bestDistanceSq) {
                    bestDistanceSq = distanceSq;
                    bestSlot = i;
                }
            }

            if (bestSlot < contourPoints.size()) {
                contourPoints[bestSlot].retarget(contour);
                claimed[bestSlot] = true;
            } else {
                contourPoints.emplace_back(contour);
                claimed.push_back(true);
            }
        }

        for (size_t i = 0; i < contourPoints.size(); ++i) {
            if (not claimed[i]) {
                contourPoints[i].deactivate();
            }
        }

        background(21, 50);

        noFill();
        stroke(255);
        strokeWeight(2);

        pushMatrix();
        translate(centerX - layout.width * 0.5f, centerY);

        for (TextContourPoints& contourPoint : contourPoints) {
            contourPoint.update(getDeltaTime());
            contourPoint.show();
        }

        popMatrix();
    }
};

std::unique_ptr<Sketch> p5cpp::createSketch()
{
    return std::make_unique<InteractiveClock>();
}
