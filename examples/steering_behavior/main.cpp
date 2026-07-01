#include <cfloat>
#include <p5cpp/p5cpp.hpp>

using namespace p5cpp;

class Vehicle
{
public:
    float2 position;
    float2 velocity;
    float2 acceleration;
    float maxSpeed = randomFloat(100.0f, 300.0f);
    float maxForce = randomFloat(7000.0f, 9000.0f);
    float lifetime = randomFloat(10.0f, 30.0f);
    float timeAlive = 0.0f;
    float size = randomFloat(16.0f, 32.0f);
    bool shouldReproduce = false;

    float foodAttractionStrength = randomFloat(-1.0f, 1.0f);
    float poisonAttractionStrength = randomFloat(-1.0f, 1.0f);
    float viewRadius = randomFloat(75.0f, 150.0f);

    explicit Vehicle(float2 position)
        : position(position),
          velocity(50.0f, 100.0f),
          acceleration(0.0f, 0.0f)
    {
    }

    Vehicle reproduce()
    {
        Vehicle child = *this;
        child.foodAttractionStrength += randomFloat(-0.1f, 0.1f);
        child.poisonAttractionStrength += randomFloat(-0.1f, 0.1f);
        child.viewRadius += randomFloat(-10.0f, 10.0f);
        child.maxSpeed += randomFloat(-20.0f, 20.0f);
        child.maxForce += randomFloat(-50.0f, 50.0f);
        child.size += randomFloat(-2.0f, 2.0f);

        return child;
    }

    void eat(std::vector<float2>& food, std::vector<float2>& poison)
    {
        if (const auto nearestFoodIndex = getNearestTargetIndex(food)) {
            const float2& foodPosition = food.at(nearestFoodIndex.value());
            const float2 steer = seek(foodPosition) * foodAttractionStrength;
            applyForce(steer);
            if (consume(food, nearestFoodIndex.value())) {
                lifetime += 5.0f;
                shouldReproduce = randomFloat(1.0f) < 0.2f; // 20% chance to reproduce after eating
            }
        }

        if (const auto nearestPoisonIndex = getNearestTargetIndex(poison)) {
            const float2& poisonPosition = poison.at(nearestPoisonIndex.value());
            const float2 steer = seek(poisonPosition) * poisonAttractionStrength;
            applyForce(steer);
            if (consume(poison, nearestPoisonIndex.value())) {
                lifetime -= 5.0f;
            }
        }
    }

    void update(float deltaTime)
    {
        timeAlive += deltaTime;

        velocity += acceleration * deltaTime;
        velocity = velocity.limited(maxSpeed);

        position += velocity * deltaTime;
        acceleration = float2(0.0f, 0.0f);

        if (position.x < 0) position.x = getWidth();
        if (position.x > getWidth()) position.x = 0;
        if (position.y < 0) position.y = getHeight();
        if (position.y > getHeight()) position.y = 0;
    }

    void show()
    {
        const float headingAngle = std::atan2(velocity.y, velocity.x);
        const float halfSize = size / 2.0f;

        pushMatrix();
        translate(position.x, position.y);
        rotate(headingAngle);
        stroke(255);
        strokeWeight(1.0f);
        fill(255, 100);
        triangle(halfSize, 0.0f, -halfSize, halfSize * 0.5f, -halfSize, -halfSize * 0.5f);
        popMatrix();

        // const float2 direction = normalized(velocity) * 10.0f;

        // stroke(255);
        // strokeWeight(2.0f);
        // line(position.x, position.y, position.x + direction.x * 10.0f, position.y + direction.y * 10.0f);

        noFill();
        stroke(0, 255, 255, 100);
        strokeWeight(3.0f);
        circle(position.x, position.y, viewRadius * 2.0f);

        const float halfCircle = 3.141592f;
        const float foodAttractionFactor = remap(foodAttractionStrength, -1.0f, 1.0f, -halfCircle, halfCircle);
        const float poisonAttractionFactor = remap(poisonAttractionStrength, -1.0f, 1.0f, -halfCircle, halfCircle);

        const float foodIndicatorRadius = viewRadius + 5.0f;
        const float poisonIndicatorRadius = viewRadius + 15.0f;

        noFill();
        strokeWeight(4.0f);
        stroke(0, 255, 0, 100);
        arc(
            position.x,
            position.y,
            foodIndicatorRadius * 2.0f,
            foodIndicatorRadius * 2.0f,
            -3.141592f / 2.0f,
            foodAttractionFactor,
            ArcMode::open
        );
        stroke(255, 0, 0, 100);
        arc(
            position.x,
            position.y,
            poisonIndicatorRadius * 2.0f,
            poisonIndicatorRadius * 2.0f,
            -3.141592f / 2.0f,
            poisonAttractionFactor,
            ArcMode::open
        );

        const float healthFactor = remap(timeAlive, 0.0f, lifetime, 1.0f, 0.0f);
        const float healthIndicatorRadius = viewRadius + 25.0f;
        const color_t healthColor = lerp(rgba(255, 0, 0), rgba(0, 255, 0), healthFactor);

        stroke(healthColor);
        strokeWeight(4.0f);
        noFill();
        arc(
            position.x,
            position.y,
            healthIndicatorRadius * 2.0f,
            healthIndicatorRadius * 2.0f,
            -3.141592f / 2.0f,
            healthFactor * 2.0f * 3.141592f,
            ArcMode::open
        );
    }

private:
    float2 seek(float2 target)
    {
        float2 desired = target - position;
        desired = desired.normalized() * maxSpeed;

        float2 steer = desired - velocity;
        steer = steer.limited(maxForce);

        return steer;
    }

    bool consume(std::vector<float2>& targets, size_t index)
    {
        const float distance = (targets[index] - position).length();
        if (distance < size) {
            targets.erase(targets.begin() + index);
            return true;
        }

        return false;
    }

    std::optional<size_t> getNearestTargetIndex(const std::span<const float2>& targets)
    {
        std::optional<size_t> closestIndex;
        float closestDistance = FLT_MAX;

        for (size_t i = 0; i < targets.size(); ++i) {
            const float distance = (targets[i] - position).length();
            if (distance > viewRadius) {
                continue;
            }

            if (distance < closestDistance) {
                closestDistance = distance;
                closestIndex = i;
            }
        }

        return closestIndex;
    }

    void applyForce(float2 force)
    {
        acceleration += force;
    }
};

struct SteeringBehavior : Sketch
{
    std::vector<Vehicle> vehicles;
    std::vector<float2> food;
    std::vector<float2> poison;

    float foodSpawnTimer = 0.0f;
    float poisonSpawnTimer = 0.0f;

    void setup() override
    {
        setWindowSize(1024, 1024);

        for (size_t i = 0; i < 5; ++i) {
            const float x = randomFloat(getWidth());
            const float y = randomFloat(getHeight());
            vehicles.push_back(Vehicle {float2(x, y)});
        }

        for (size_t i = 0; i < 20; ++i) {
            const float x = randomFloat(getWidth());
            const float y = randomFloat(getHeight());
            food.push_back(float2(x, y));
        }

        for (size_t i = 0; i < 20; ++i) {
            const float x = randomFloat(getWidth());
            const float y = randomFloat(getHeight());
            poison.push_back(float2(x, y));
        }
    }

    void event(const WindowEvent& e) override
    {
    }

    void draw() override
    {
        background(21);

        foodSpawnTimer += getDeltaTime();
        poisonSpawnTimer += getDeltaTime();

        if (foodSpawnTimer >= 0.5f) {
            foodSpawnTimer = 0.0f;
            const float x = randomFloat(getWidth());
            const float y = randomFloat(getHeight());
            food.push_back(float2(x, y));
        }

        if (poisonSpawnTimer >= 1.0f) {
            poisonSpawnTimer = 0.0f;
            const float x = randomFloat(getWidth());
            const float y = randomFloat(getHeight());
            poison.push_back(float2(x, y));
        }

        const float2 mouse = {static_cast<float>(getMouseX()), static_cast<float>(getMouseY())};
        for (Vehicle& vehicle : vehicles) {
            vehicle.eat(food, poison);
            vehicle.update(getDeltaTime());
            vehicle.show();
        }

        // Repdocue
        const size_t initialVehicleCount = vehicles.size();
        for (size_t i = 0; i < initialVehicleCount; ++i) {
            Vehicle& vehicle = vehicles.at(i);

            if (vehicle.shouldReproduce) {
                vehicle.shouldReproduce = false;
                vehicles.push_back(vehicle.reproduce());
            }
        }

        for (size_t i = vehicles.size(); i-- > 0;) {
            Vehicle& vehicle = vehicles.at(i);
            const bool isDead = vehicle.timeAlive >= vehicle.lifetime;
            if (not isDead) {
                continue;
            }

            vehicles.erase(vehicles.begin() + i);
        }

        for (const float2& foodPos : food) {
            fill(0, 255, 0);
            noStroke();
            circle(foodPos.x, foodPos.y, 8.0f);
        }

        for (const float2& poisonPos : poison) {
            fill(255, 0, 0);
            noStroke();
            circle(poisonPos.x, poisonPos.y, 8.0f);
        }

        fill(255);
        textSize(24.0f);
        textAlign(TextAlign::topLeft);
        text("Vehicle count: " + std::to_string(vehicles.size()), 10.0f, 10.0f);
        text("Food count: " + std::to_string(food.size()), 10.0f, 40.0f);
        text("Poison count: " + std::to_string(poison.size()), 10.0f, 70.0f);
    }
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<SteeringBehavior>();
    }
} // namespace p5cpp
