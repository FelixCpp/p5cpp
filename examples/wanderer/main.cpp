#include <algorithm>
#include <array>
#include <functional>
#include <optional>
#include <utility>
#include <vector>
#include <p5cpp/p5cpp.hpp>

using namespace p5;

enum Direction
{
    left,
    up,
    right,
    down,
};

using Palette = std::array<color_t, 5>;

inline static constexpr Palette autumnPalette = {
    rgba(255, 93, 0),
    rgba(255, 129, 0),
    rgba(255, 166, 0),
    rgba(255, 191, 0),
    rgba(255, 216, 0),
};

inline static constexpr Palette oceanPalette = {
    rgba(3, 4, 94),
    rgba(0, 119, 182),
    rgba(0, 180, 216),
    rgba(72, 202, 228),
    rgba(144, 224, 239),
};

inline static constexpr Palette neonPalette = {
    rgba(255, 0, 110),
    rgba(131, 56, 236),
    rgba(58, 134, 255),
    rgba(255, 190, 11),
    rgba(251, 86, 7),
};

inline static constexpr Palette forestPalette = {
    rgba(45, 106, 79),
    rgba(52, 160, 106),
    rgba(82, 182, 154),
    rgba(116, 198, 157),
    rgba(183, 228, 199),
};

inline static constexpr Palette sunsetPalette = {
    rgba(247, 37, 133),
    rgba(114, 9, 183),
    rgba(58, 12, 163),
    rgba(67, 97, 238),
    rgba(76, 201, 240),
};

inline static constexpr Palette pastelPalette = {
    rgba(255, 173, 173),
    rgba(255, 214, 165),
    rgba(253, 255, 182),
    rgba(202, 255, 191),
    rgba(155, 246, 255),
};

inline static constexpr Palette volcanoPalette = {
    rgba(208, 0, 0),
    rgba(224, 86, 36),
    rgba(255, 186, 8),
    rgba(210, 0, 98),
    rgba(157, 2, 8),
};

inline static constexpr Palette desertPalette = {
    rgba(231, 111, 81),
    rgba(244, 162, 97),
    rgba(233, 196, 106),
    rgba(42, 157, 143),
    rgba(38, 70, 83),
};

inline static constexpr Palette synthwavePalette = {
    rgba(254, 218, 118),
    rgba(250, 112, 154),
    rgba(254, 81, 150),
    rgba(154, 0, 254),
    rgba(10, 189, 227),
};

inline static constexpr Palette nordicPalette = {
    rgba(141, 153, 174),
    rgba(239, 35, 60),
    rgba(217, 4, 41),
    rgba(72, 202, 228),
    rgba(248, 249, 250),
};

inline static const std::array<Palette, 10> colorPalettes = {
    autumnPalette,
    oceanPalette,
    neonPalette,
    forestPalette,
    sunsetPalette,
    pastelPalette,
    volcanoPalette,
    desertPalette,
    synthwavePalette,
    nordicPalette,
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
    struct MovementPoint
    {
        float2 position;
        bool isEnd;
    };

    explicit Wanderer(const float2& position, const color_t color, const Direction direction, const float directionChangeInterval)
        : m_directionChangeInterval(directionChangeInterval),
          m_timeElapsedSinceLastDirectionChange(0.0f),
          m_direction(direction),
          m_color(color),
          m_position(position),
          m_movementPoints({{position, false}})
    {
    }

    void pruneTrail(const float maxTrailLength)
    {
        float totalLength = 0.0f;
        for (size_t i = 1; i < m_movementPoints.size(); ++i) {
            if (not m_movementPoints[i - 1].isEnd) {
                totalLength += length(m_movementPoints[i].position - m_movementPoints[i - 1].position);
            }
        }
        if (not m_movementPoints.empty() and not m_movementPoints.back().isEnd) {
            totalLength += length(m_position - m_movementPoints.back().position);
        }

        while (totalLength > maxTrailLength and not m_movementPoints.empty()) {
            const float excess = totalLength - maxTrailLength;

            if (m_movementPoints[0].isEnd) {
                m_movementPoints.erase(m_movementPoints.begin());
                continue;
            }

            const float2 nextPos = (m_movementPoints.size() > 1) ? m_movementPoints[1].position : m_position;
            const float2 diff = nextPos - m_movementPoints[0].position;
            const float segLen = length(diff);

            if (segLen <= excess or segLen < 0.0001f) {
                totalLength -= segLen;
                m_movementPoints.erase(m_movementPoints.begin());
            } else {
                const float2 dir = diff / segLen;
                m_movementPoints[0].position = m_movementPoints[0].position + dir * excess;
                totalLength -= excess;
                break;
            }
        }

        if (m_movementPoints.empty()) {
            m_movementPoints.push_back({m_position, false});
        }
    }
    void moveStep(const rect2f& boundary, const float deltaTime)
    {
        m_timeElapsedSinceLastDirectionChange += deltaTime;

        if (m_timeElapsedSinceLastDirectionChange >= m_directionChangeInterval) {
            changeDirection();
            m_directionChangeInterval = random(0.1f, 0.3f);
            m_timeElapsedSinceLastDirectionChange = 0.0f;
        }

        move(boundary, deltaTime);
        pruneTrail(1000.0f);
    }

    void show() const
    {
        stroke(m_color);
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

            line(last.position.x, last.position.y, m_position.x, m_position.y);
        }
    }

    const std::vector<MovementPoint>& getMovementPoints() const { return m_movementPoints; }
    std::vector<MovementPoint>& getMovementPoints() { return m_movementPoints; }
    const float2& getPosition() const { return m_position; }

    bool hasActiveHeadSegment() const
    {
        return not m_movementPoints.empty() and not m_movementPoints.back().isEnd;
    }

    std::pair<float2, float2> getHeadSegment() const
    {
        return {m_movementPoints.back().position, m_position};
    }

    void resolveIntersectionAt(const size_t fromIndex, const float2& intersectionPos, const float2& lineEnd)
    {
        const float2 lineStart = m_movementPoints.at(fromIndex).position;
        const float2 diffToLineStart = lineStart - intersectionPos;
        const float2 diffToLineEnd = lineEnd - intersectionPos;
        const float2 directionToLineStart = normalized(diffToLineStart);
        const float2 directionToLineEnd = normalized(diffToLineEnd);

        constexpr float maxGapSize = 10.0f;
        const float distanceToLineStart = length(diffToLineStart);
        const float distanceToLineEnd = length(diffToLineEnd);

        size_t insertOffset = 1;

        if (distanceToLineStart > 0.0f) {
            const float gapSizeTowardsLineStart = std::min(distanceToLineStart, maxGapSize);
            const float2 intersectionGapTowardsStart = intersectionPos + directionToLineStart * gapSizeTowardsLineStart;
            m_movementPoints.insert(m_movementPoints.begin() + fromIndex + insertOffset, MovementPoint {.position = intersectionGapTowardsStart, .isEnd = true});
            ++insertOffset;
        } else {
            m_movementPoints.at(fromIndex).isEnd = true;
        }

        if (distanceToLineEnd > 0.0f) {
            const float gapSizeTowardsLineEnd = std::min(distanceToLineEnd, maxGapSize);
            const float2 intersectionGapTowardsEnd = intersectionPos + directionToLineEnd * gapSizeTowardsLineEnd;
            m_movementPoints.insert(m_movementPoints.begin() + fromIndex + insertOffset, MovementPoint {.position = intersectionGapTowardsEnd, .isEnd = false});
        }
    }

private:
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
        const float speed = 150.0f * deltaTime;
        if (m_direction == Direction::left) m_position.x -= speed;
        if (m_direction == Direction::up) m_position.y -= speed;
        if (m_direction == Direction::right) m_position.x += speed;
        if (m_direction == Direction::down) m_position.y += speed;

        if (m_position.x < boundary.left) {
            m_movementPoints.push_back({float2 {boundary.left, m_position.y}, true});
            m_position.x = boundary.left + boundary.width;
            m_movementPoints.push_back({m_position, false});
        }

        if (m_position.x > boundary.left + boundary.width) {
            m_movementPoints.push_back({float2 {boundary.left + boundary.width, m_position.y}, true});
            m_position.x = boundary.left;
            m_movementPoints.push_back({m_position, false});
        }

        if (m_position.y < boundary.top) {
            m_movementPoints.push_back({float2 {m_position.x, boundary.top}, true});
            m_position.y = boundary.top + boundary.height;
            m_movementPoints.push_back({m_position, false});
        }

        if (m_position.y > boundary.top + boundary.height) {
            m_movementPoints.push_back({float2 {m_position.x, boundary.top + boundary.height}, true});
            m_position.y = boundary.top;
            m_movementPoints.push_back({m_position, false});
        }
    }

    float m_directionChangeInterval;
    float m_timeElapsedSinceLastDirectionChange;

    Direction m_direction;
    color_t m_color;

    float2 m_position;
    std::vector<MovementPoint> m_movementPoints;
};

class GameField
{
public:
    explicit GameField(const rect2f& boundary)
        : m_boundary(boundary)
    {
        const size_t paletteIndex = static_cast<size_t>(random(0, colorPalettes.size()));
        const Palette& palette = colorPalettes.at(paletteIndex);

        for (size_t i = 0; i < palette.size(); ++i) {
            const float2 randomPosition {
                .x = random(boundary.left + 10.0f, boundary.left + boundary.width - 10.0f),
                .y = random(boundary.top + 10.0f, boundary.top + boundary.height - 10.0f),
            };

            const Direction randomDirection = static_cast<Direction>(static_cast<int>(random(0, 4)));

            wanderers.emplace_back(randomPosition, palette.at(i), randomDirection, 1.0f);
        }
    }

    void update(const float deltaTime)
    {
        struct Intersection
        {
            size_t targetWandererIndex;
            size_t fromMovementPointIndex;
            float2 position;
            float distanceToLineStart;
        };

        for (size_t i = 0; i < wanderers.size(); ++i) {
            wanderers[i].moveStep(m_boundary, deltaTime);

            if (not wanderers[i].hasActiveHeadSegment()) {
                continue;
            }

            const auto [hStart, hEnd] = wanderers[i].getHeadSegment();
            std::vector<Intersection> intersections;

            for (size_t j = 0; j < wanderers.size(); ++j) {
                const auto& pts = wanderers[j].getMovementPoints();
                if (pts.empty()) {
                    continue;
                }

                const size_t numPts = pts.size();
                for (size_t k = 0; k < numPts; ++k) {
                    if (pts[k].isEnd) {
                        continue;
                    }

                    if (i == j) {
                        if (k == numPts - 1) {
                            continue;
                        }
                        if (k == numPts - 2) {
                            continue;
                        }
                    }

                    const float2 lineStart = pts[k].position;
                    const float2 lineEnd = (k < numPts - 1) ? pts[k + 1].position : wanderers[j].getPosition();

                    const std::optional<float2> intersectionPos = getLineToLineIntersection(lineStart, lineEnd, hStart, hEnd);
                    if (intersectionPos.has_value()) {
                        const float distToStart = length(lineStart - intersectionPos.value());
                        intersections.push_back(Intersection {
                            .targetWandererIndex = j,
                            .fromMovementPointIndex = k,
                            .position = intersectionPos.value(),
                            .distanceToLineStart = distToStart,
                        });
                    }
                }
            }

            std::sort(intersections.begin(), intersections.end(), [](const Intersection& a, const Intersection& b) {
                if (a.targetWandererIndex != b.targetWandererIndex) {
                    return a.targetWandererIndex < b.targetWandererIndex;
                }
                if (a.fromMovementPointIndex != b.fromMovementPointIndex) {
                    return a.fromMovementPointIndex > b.fromMovementPointIndex;
                }
                return a.distanceToLineStart > b.distanceToLineStart;
            });

            for (const auto& inter : intersections) {
                Wanderer& target = wanderers.at(inter.targetWandererIndex);
                const auto& pts = target.getMovementPoints();
                const size_t k = inter.fromMovementPointIndex;
                if (k >= pts.size()) {
                    continue;
                }
                const float2 lineEnd = (k < pts.size() - 1) ? pts.at(k + 1).position : target.getPosition();
                target.resolveIntersectionAt(k, inter.position, lineEnd);
            }
        }
    }

    void show()
    {
        for (Wanderer& wanderer : wanderers) {
            wanderer.show();
        }

        noFill();
        stroke(rgba(255, 100));
        strokeWeight(1.0f);
        rect(m_boundary.left, m_boundary.top, m_boundary.width, m_boundary.height, BorderRadius::all(5.0f));
    }

private:
    std::vector<Wanderer> wanderers;
    rect2f m_boundary;
};

struct WandererSketch : Sketch
{
    std::vector<GameField> gameFields;

    void setup() override
    {
        const int totalWindowWidth = 900;
        const int totalWindowHeight = 900;
        setWindowSize(totalWindowWidth, totalWindowHeight);

        const size_t columns = 4;
        const size_t rows = 2;
        const float windowMargin = 10.0f;
        const float gameFieldSpacing = 10.0f;
        const float gameFieldWidth = (totalWindowWidth - windowMargin * 2.0f - gameFieldSpacing * (columns - 1)) / columns;
        const float gameFieldHeight = (totalWindowHeight - windowMargin * 2.0f - gameFieldSpacing * (rows - 1)) / rows;

        for (size_t row = 0; row < rows; ++row) {
            for (size_t column = 0; column < columns; ++column) {
                const float left = windowMargin + column * (gameFieldWidth + gameFieldSpacing);
                const float top = windowMargin + row * (gameFieldHeight + gameFieldSpacing);
                const rect2f gameFieldBoundary {left, top, gameFieldWidth, gameFieldHeight};
                gameFields.push_back(GameField {gameFieldBoundary});
            }
        }
    }

    void draw() override
    {
        const float deltaTime = getDeltaTime();

        for (GameField& gameField : gameFields) {
            gameField.update(deltaTime);
        }

        background(rgba(21, 21, 31));
        for (GameField& gameField : gameFields) {
            gameField.show();
        }
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
