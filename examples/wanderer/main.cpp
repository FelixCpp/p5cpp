#include <functional>
#include <p5cpp/p5cpp.hpp>

using namespace p5;

enum Direction
{
    left,
    up,
    right,
    down,
};

inline static constexpr std::array<color_t, 5> colorPalette = {
    rgba(255, 93, 0),
    rgba(255, 129, 0),
    rgba(255, 166, 0),
    rgba(255, 191, 0),
    rgba(255, 216, 0),
};

inline static constexpr std::optional<float2> getLineToLineIntersection(const float2& p1, const float2& p2, const float2& p3, const float2& p4)
{
    const float denominator = (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x);
    if (denominator == 0.0f) {
        return std::nullopt;
    }

    const float t = ((p1.x - p3.x) * (p3.y - p4.y) - (p1.y - p3.y) * (p3.x - p4.x)) / denominator;
    const float u = -((p1.x - p2.x) * (p1.y - p3.y) - (p1.y - p2.y) * (p1.x - p3.x)) / denominator;

    if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f) {
        return float2 {
            .x = p1.x + t * (p2.x - p1.x),
            .y = p1.y + t * (p2.y - p1.y),
        };
    }

    return std::nullopt;
}

class Wanderer
{
public:
    explicit Wanderer(const float2& position, float directionChangeInterval)
        : m_directionChangeInterval(directionChangeInterval),
          m_timeElapsedSinceLastDirectionChange(0.0f),
          m_direction(Direction::right),
          m_position(position),
          m_movementPoints({{position, false}})
    {
    }

    void update(const rect2f& boundary, const float deltaTime)
    {
        m_timeElapsedSinceLastDirectionChange += deltaTime;

        if (m_timeElapsedSinceLastDirectionChange >= m_directionChangeInterval) {
            changeDirection();
            m_directionChangeInterval = random(0.5f, 2.0f);
            m_timeElapsedSinceLastDirectionChange = 0.0f;
        }

        move(boundary, deltaTime);

        const std::vector<Intersection> intersections = resolveNewestIntersections();
        for (auto it = intersections.rbegin(); it != intersections.rend(); ++it) {
            resolveIntersection(*it);
        }
    }

    void show()
    {
        stroke(colorPalette.at(1));
        strokeWeight(2.0f);
        noFill();

        for (size_t i = 1; i < m_movementPoints.size(); ++i) {
            const MovementPoint& previous = m_movementPoints.at(i - 1);
            if (previous.isEnd) {
                continue;
            }

            const MovementPoint& current = m_movementPoints.at(i);
            line(previous.position.x, previous.position.y, current.position.x, current.position.y);
        }


        if (not m_movementPoints.empty()) {
            const MovementPoint& last = m_movementPoints.back();
            if (last.isEnd) {
                return;
            }

            stroke(colorPalette.at(2));
            strokeWeight(2.0f);
            noFill();
            line(last.position.x, last.position.y, m_position.x, m_position.y);
        }


        for (const Intersection& intersection : resolveNewestIntersections()) {
            const MovementPoint& a = m_movementPoints.at(intersection.fromMovementPointIndex);
            const MovementPoint& b = m_movementPoints.at(intersection.toMovementPointIndex);
            stroke(rgba(0, 255, 0));
            strokeWeight(2.0f);
            line(a.position.x, a.position.y, b.position.x, b.position.y);

            noStroke();
            fill(rgba(255));
            circle(intersection.position.x, intersection.position.y, 2.0f);
        }
    }

private:
    struct MovementPoint
    {
        float2 position;
        bool isEnd;
    };

    struct Intersection
    {
        float2 position;
        size_t fromMovementPointIndex;
        size_t toMovementPointIndex;
    };

    std::vector<Intersection> resolveNewestIntersections()
    {
        if (m_movementPoints.empty()) {
            return {};
        }

        const MovementPoint& last = m_movementPoints.back();
        if (last.isEnd) {
            return {};
        }

        std::vector<Intersection> intersections;

        for (size_t i = 1; i < m_movementPoints.size(); ++i) {
            const MovementPoint& previous = m_movementPoints.at(i - 1);
            if (previous.isEnd) {
                continue;
            }

            const MovementPoint& current = m_movementPoints.at(i);
            const std::optional<float2> intersectionWithHead = getLineToLineIntersection(previous.position, current.position, last.position, m_position);
            if (not intersectionWithHead.has_value()) {
                continue;
            }

            intersections.emplace_back(Intersection {
                .position = intersectionWithHead.value(),
                .fromMovementPointIndex = i - 1,
                .toMovementPointIndex = i,
            });
        }

        return intersections;
    }

    void resolveIntersection(const Intersection& intersection)
    {
        const float2 lineStart = m_movementPoints.at(intersection.fromMovementPointIndex).position;
        const float2 lineEnd = m_movementPoints.at(intersection.toMovementPointIndex).position;
        const float2 diffToLineStart = lineStart - intersection.position;
        const float2 diffToLineEnd = lineEnd - intersection.position;
        const float2 directionToLineStart = normalized(diffToLineStart);
        const float2 directionToLineEnd = normalized(diffToLineEnd);

        constexpr float maxGapSize = 10.0f;
        const float distanceToLineStart = length(diffToLineStart);
        const float distanceToLineEnd = length(diffToLineEnd);

        size_t insertOffset = 1;

        if (distanceToLineStart > 0.0f) {
            const float gapSizeTowardsLineStart = std::min(distanceToLineStart, maxGapSize);
            const float2 intersectionGapTowardsStart = intersection.position + directionToLineStart * gapSizeTowardsLineStart;
            m_movementPoints.insert(m_movementPoints.begin() + intersection.fromMovementPointIndex + insertOffset, MovementPoint {.position = intersectionGapTowardsStart, .isEnd = true});
            ++insertOffset;
        }

        if (distanceToLineEnd > 0.0f) {
            const float gapSizeTowardsLineEnd = std::min(distanceToLineEnd, maxGapSize);
            const float2 intersectionGapTowardsEnd = intersection.position + directionToLineEnd * gapSizeTowardsLineEnd;
            m_movementPoints.insert(m_movementPoints.begin() + intersection.fromMovementPointIndex + insertOffset, MovementPoint {.position = intersectionGapTowardsEnd, .isEnd = false});
        }
    }

    Direction getNewDirection()
    {
        const std::array<Direction, 2> possibleDirections = std::invoke([this] -> std::array<Direction, 2> {
            const bool isHorizontal = m_direction == Direction::left or m_direction == Direction::right;
            if (isHorizontal) {
                return {Direction::up, Direction::down};
            } else {
                return {Direction::left, Direction::right};
            }
        });

        const size_t randomDirectionIndex = static_cast<size_t>(random(0, possibleDirections.size()));
        return possibleDirections.at(randomDirectionIndex);
    }

    void changeDirection()
    {
        m_direction = getNewDirection();
        m_movementPoints.push_back({m_position, false});
    }

    void move(const rect2f& boundary, const float deltaTime)
    {
        const float speed = 300.0f * deltaTime;
        if (m_direction == Direction::left) m_position.x -= speed;
        if (m_direction == Direction::up) m_position.y -= speed;
        if (m_direction == Direction::right) m_position.x += speed;
        if (m_direction == Direction::down) m_position.y += speed;

        if (m_position.x < boundary.left) {
            m_movementPoints.push_back({m_position, true});
            m_position.x = boundary.left + boundary.width;
            m_movementPoints.push_back({m_position, false});
        }

        if (m_position.x > boundary.left + boundary.width) {
            m_movementPoints.push_back({m_position, true});
            m_position.x = boundary.left;
            m_movementPoints.push_back({m_position, false});
        }

        if (m_position.y < boundary.top) {
            m_movementPoints.push_back({m_position, true});
            m_position.y = boundary.top + boundary.height;
            m_movementPoints.push_back({m_position, false});
        }

        if (m_position.y > boundary.top + boundary.height) {
            m_movementPoints.push_back({m_position, true});
            m_position.y = boundary.top;
            m_movementPoints.push_back({m_position, false});
        }
    }

    float m_directionChangeInterval;
    float m_timeElapsedSinceLastDirectionChange;

    Direction m_direction;

    float2 m_position;
    std::vector<MovementPoint> m_movementPoints;
};

struct WandererSketch : Sketch
{
    Wanderer wanderer {float2 {.x = 450.0f, .y = 450.0f}, 1.0f};

    void setup() override
    {
        setWindowSize(900, 900);
    }

    void draw() override
    {
        const float deltaTime = getDeltaTime();
        const rect2f boundary {100.0f, 100.0f, 700.0f, 700.0f};

        wanderer.update(boundary, deltaTime);

        background(rgba(21, 21, 31));
        wanderer.show();

        noFill();
        stroke(rgba(255));
        strokeWeight(4.0f);
        rect(boundary.left, boundary.top, boundary.width, boundary.height, BorderRadius::all(15.0f));
    }
};

SketchSpec p5::createSpec()
{
    return {
        .sketch = [] {
            return std::make_unique<WandererSketch>();
        }
    };
}
