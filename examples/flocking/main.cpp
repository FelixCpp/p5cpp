#include <format>

#include <p5cpp/p5cpp.hpp>
using namespace p5cpp;

struct Boid;

struct MovementBehavior
{
    virtual ~MovementBehavior() = default;
    virtual float2 apply(const Boid& boid, std::span<const Boid*>& neighbors) = 0;
};

static color_t getRandomColor()
{
    const float hue = randomFloat(0.0f, 360.0f);
    return hsv(hue, 1.0f, 1.0f, 255);
}

struct Boid
{
    float2 position;
    float2 velocity;
    float2 acceleration;
    float maxAccelerationStrength;
    float maxVelocityStrength;
    color_t color;

    explicit Boid(const float x, const float y)
        : position(x, y), maxAccelerationStrength(50.0f), maxVelocityStrength(100.0f), color(getRandomColor())
    {
    }

    void applyForce(const float2& force)
    {
        acceleration += force;
    }

    float2 steer(const float2& desired) const
    {
        return limited(normalized(desired) * maxVelocityStrength - velocity, maxAccelerationStrength);
    }

    void update(float deltaTime)
    {
        velocity = limited(velocity + acceleration * deltaTime, maxVelocityStrength);
        position += velocity * deltaTime;
        acceleration *= 0.0f;
    }

    void wrapAround(float width, float height)
    {
        if (position.x < 0) position.x += width;
        if (position.x > width) position.x -= width;
        if (position.y < 0) position.y += height;
        if (position.y > height) position.y -= height;
    }

    void show() const
    {
        float angle = std::atan2(velocity.y, velocity.x);
        const float triangleSize = 8.0f;

        // const float hue = std::fmod(position.x + position.y, 360.0f);
        // const color_t fillColor = hsv(hue, 1.0f, 1.0f, 255);

        pushMatrix();
        translate(position.x, position.y);
        rotate(angle);
        noStroke();
        fill(color);
        triangle(-triangleSize, triangleSize / 2.0f, -triangleSize, -triangleSize / 2.0f, triangleSize, 0.0f);
        popMatrix();
    }
};

struct AlignmentBehavior : MovementBehavior
{
    float weight;

    explicit AlignmentBehavior(float weight)
        : weight(weight)
    {
    }

    float2 apply(const Boid& boid, std::span<const Boid*>& neighbors) override
    {
        if (neighbors.empty()) {
            return float2::zero;
        }

        float2 steering(0.0f, 0.0f);

        for (const Boid* neighbor : neighbors) {
            steering += neighbor->velocity;
        }

        const float2 averageVelocity = steering / static_cast<float>(neighbors.size());
        return boid.steer(averageVelocity) * weight;
    }
};

struct SeparationBehavior : MovementBehavior
{
    float weight;

    explicit SeparationBehavior(float weight)
        : weight(weight)
    {
    }

    float2 apply(const Boid& boid, std::span<const Boid*>& neighbors) override
    {
        if (neighbors.empty()) {
            return float2::zero;
        }

        float2 steering(0.0f, 0.0f);
        size_t total = 0;

        for (const Boid* neighbor : neighbors) {
            float2 diff = boid.position - neighbor->position;
            float distance = length(diff);

            if (distance > 0.0f) {
                diff /= distance;
                steering += diff;
                ++total;
            }
        }

        const float2 separationForce = steering / static_cast<float>(total);
        return boid.steer(separationForce) * weight;
    }
};

struct CohesionBehavior : MovementBehavior
{
    float weight;

    explicit CohesionBehavior(float weight)
        : weight(weight)
    {
    }

    float2 apply(const Boid& boid, std::span<const Boid*>& neighbors) override
    {
        if (neighbors.empty()) {
            return float2::zero;
        }

        float2 steering(0.0f, 0.0f);

        for (const Boid* neighbor : neighbors) {
            steering += neighbor->position;
        }

        const float2 averagePosition = steering / static_cast<float>(neighbors.size());
        const float2 desired = averagePosition - boid.position;
        return boid.steer(desired) * weight;
    }
};

struct QuadTreePoint
{
    float x;
    float y;
    void* userData;

    explicit QuadTreePoint(const float2& position, void* userData)
        : x(position.x), y(position.y), userData(userData)
    {
    }
};

struct QuadTreeCircularQuery
{
    float x;
    float y;
    float radius;

    explicit QuadTreeCircularQuery(const float x, const float y, float radius)
        : x(x), y(y), radius(radius)
    {
    }

    bool contains(const float pointX, const float pointY) const
    {
        const float dx = pointX - x;
        const float dy = pointY - y;
        return (dx * dx + dy * dy) <= (radius * radius);
    }

    bool intersects(const float_rect& boundary) const
    {
        const float closestX = std::clamp(x, boundary.left, boundary.left + boundary.width);
        const float closestY = std::clamp(y, boundary.top, boundary.top + boundary.height);
        const float dx = x - closestX;
        const float dy = y - closestY;
        return (dx * dx + dy * dy) <= (radius * radius);
    }
};

struct QuadTree
{
    explicit QuadTree(const float_rect& boundary, size_t capacity)
        : boundary(boundary), capacity(capacity)
    {
    }

    bool insert(float x, float y, void* userData)
    {
        if (not boundary.contains(x, y)) {
            return false;
        }

        if (points.size() >= capacity) {
            if (not isSubdivided) {
                subdivide();
            }

            if (northeast->insert(x, y, userData)) return true;
            if (northwest->insert(x, y, userData)) return true;
            if (southeast->insert(x, y, userData)) return true;
            if (southwest->insert(x, y, userData)) return true;

            return false; // This should not happen if the point is within the boundary
        }

        points.emplace_back(float2(x, y), userData);
        return true;
    }

    void query(const QuadTreeCircularQuery& query, auto onInsert) const
    {
        if (not query.intersects(boundary)) {
            return;
        }

        for (const QuadTreePoint& point : points) {
            if (query.contains(point.x, point.y)) {
                onInsert(point);
            }
        }

        if (isSubdivided) {
            northeast->query(query, onInsert);
            northwest->query(query, onInsert);
            southeast->query(query, onInsert);
            southwest->query(query, onInsert);
        }
    }

    void show() const
    {
        noFill();
        stroke(255);
        strokeWeight(1.0f);
        rect(boundary.left, boundary.top, boundary.width, boundary.height);
    }

    void subdivide()
    {
        const float halfWidth = boundary.width / 2.0f;
        const float halfHeight = boundary.height / 2.0f;

        northeast = std::make_unique<QuadTree>(float_rect(boundary.left + halfWidth, boundary.top, halfWidth, halfHeight), capacity);
        northwest = std::make_unique<QuadTree>(float_rect(boundary.left, boundary.top, halfWidth, halfHeight), capacity);
        southeast = std::make_unique<QuadTree>(float_rect(boundary.left + halfWidth, boundary.top + halfHeight, halfWidth, halfHeight), capacity);
        southwest = std::make_unique<QuadTree>(float_rect(boundary.left, boundary.top + halfHeight, halfWidth, halfHeight), capacity);

        isSubdivided = true;
    }

    float_rect boundary;
    size_t capacity;

    std::vector<QuadTreePoint> points;

    std::unique_ptr<QuadTree> northeast;
    std::unique_ptr<QuadTree> northwest;
    std::unique_ptr<QuadTree> southeast;
    std::unique_ptr<QuadTree> southwest;
    bool isSubdivided = false;
};

struct FlockingSimulation : Sketch
{
    static constexpr float weightChangeSpeed = 1.0f;
    static constexpr float maxWeight = 5.0f;

    std::vector<Boid> boids;

    std::vector<std::unique_ptr<MovementBehavior>> behaviors;

    AlignmentBehavior* alignmentBehavior = nullptr;
    SeparationBehavior* separationBehavior = nullptr;
    CohesionBehavior* cohesionBehavior = nullptr;

    void setup() override
    {
        setWindowSize(800, 600);

        auto alignment = std::make_unique<AlignmentBehavior>(1.0f);
        auto separation = std::make_unique<SeparationBehavior>(1.5f);
        auto cohesion = std::make_unique<CohesionBehavior>(1.0f);

        alignmentBehavior = alignment.get();
        separationBehavior = separation.get();
        cohesionBehavior = cohesion.get();

        behaviors.emplace_back(std::move(alignment));
        behaviors.emplace_back(std::move(separation));
        behaviors.emplace_back(std::move(cohesion));

        for (size_t i = 0; i < 1000; ++i) {
            const float px = randomFloat(0.0f, static_cast<float>(getCanvasSize().x));
            const float py = randomFloat(0.0f, static_cast<float>(getCanvasSize().y));
            boids.emplace_back(px, py);
        }
    }

    void adjustWeight(float& weight, Key increaseKey, Key decreaseKey) const
    {
        if (isKeyDown(increaseKey)) {
            weight = std::clamp(weight + weightChangeSpeed * getDeltaTime(), 0.0f, maxWeight);
        }
        if (isKeyDown(decreaseKey)) {
            weight = std::clamp(weight - weightChangeSpeed * getDeltaTime(), 0.0f, maxWeight);
        }
    }

    void draw() override
    {
        background(21, 50);
        adjustWeight(alignmentBehavior->weight, Key::q, Key::a);
        adjustWeight(separationBehavior->weight, Key::w, Key::s);
        adjustWeight(cohesionBehavior->weight, Key::e, Key::d);

        QuadTree quadTree(float_rect(0.0f, 0.0f, static_cast<float>(getCanvasSize().x), static_cast<float>(getCanvasSize().y)), 4);

        for (Boid& boid : boids) {
            quadTree.insert(boid.position.x, boid.position.y, &boid);
        }

        // Avoid moving into the mouse position
        const float mouseAvoidanceRadius = 50.0f;
        const float mouseAvoidanceStrength = 800.0f;
        noFill();
        stroke(255, 100);
        strokeWeight(1.0f);
        circle(static_cast<float>(getMouseX()), static_cast<float>(getMouseY()), mouseAvoidanceRadius);

        for (Boid& boid : boids) {
            float2 mousePos(static_cast<float>(getMouseX()), static_cast<float>(getMouseY()));
            float2 diff = boid.position - mousePos;
            float distance = length(diff);

            if (distance < mouseAvoidanceRadius && distance > 0.0f) {
                float2 avoidanceForce = normalized(diff) * (mouseAvoidanceStrength * (1.0f - distance / mouseAvoidanceRadius));
                boid.applyForce(avoidanceForce);
            }
        }

        for (Boid& boid : boids) {
            QuadTreeCircularQuery query(boid.position.x, boid.position.y, 50.0f);
            if (isKeyDown(Key::c)) {

                noFill();
                stroke(255);
                strokeWeight(1.0f);
                circle(boid.position.x, boid.position.y, query.radius * 2.0f);
            }

            std::vector<const Boid*> neighbors;

            quadTree.query(query, [&neighbors, &boid](const QuadTreePoint& point) {
                const Boid* neighbor = static_cast<const Boid*>(point.userData);
                if (neighbor != &boid) {
                    neighbors.push_back(neighbor);
                }
            });

            std::span<const Boid*> neighborSpan(neighbors);
            for (const auto& behavior : behaviors) {
                float2 force = behavior->apply(boid, neighborSpan);
                boid.applyForce(force);
            }
        }

        for (Boid& boid : boids) {
            boid.update(getDeltaTime());
            boid.wrapAround(static_cast<float>(getCanvasSize().x), static_cast<float>(getCanvasSize().y));
        }

        for (Boid& boid : boids) {
            boid.show();
        }

        fill(255);
        textAlign(TextAlign::topLeft);
        textSize(14.0f);
        text(std::format("Alignment (Q/A): {:.2f}", alignmentBehavior->weight), 10.0f, 10.0f);
        text(std::format("Separation (W/S): {:.2f}", separationBehavior->weight), 10.0f, 30.0f);
        text(std::format("Cohesion (E/D): {:.2f}", cohesionBehavior->weight), 10.0f, 50.0f);
    }
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<FlockingSimulation>();
    }
} // namespace p5cpp
