#include <p5cpp/p5cpp.hpp>

using namespace p5cpp;

namespace
{
    struct Colorizer
    {
        virtual ~Colorizer() = default;
        virtual void update(float deltaTime) = 0;
        virtual color_t getColor(float2 position, float2 velocity) = 0;
    };

    struct HueColorizer : Colorizer
    {
        void update(float deltaTime) override
        {
            // No update needed for this colorizer
        }

        color_t getColor(float2 position, float2 velocity) override
        {
            const float hue = degrees(std::atan2(velocity.y, velocity.x)) + 180.0f;
            return hsv(hue, 1.0f, 1.0f, 255);
        }
    };

    struct GlobalHueColorizer : Colorizer
    {
        float hue;

        GlobalHueColorizer() : hue(0.0f) {}

        void update(float deltaTime) override
        {
            hue += 30.0f * deltaTime; // Adjust the speed of hue change as needed
            if (hue > 360.0f) {
                hue -= 360.0f;
            }
        }

        color_t getColor(float2 position, float2 velocity) override
        {
            return hsv(hue, 1.0f, 1.0f, 255);
        }
    };

    struct PositionBasedColorizer : Colorizer
    {
        void update(float deltaTime) override
        {
            // No update needed for this colorizer
        }

        color_t getColor(float2 position, float2 velocity) override
        {
            const float hue = std::fmod(position.x + position.y, 360.0f);
            return hsv(hue, 1.0f, 1.0f, 255);
        }
    };

    inline static std::unique_ptr<Colorizer> s_colorizer;

    template <typename T>
    struct QuadTreePoint
    {
        float x;
        float y;
        T* userData;

        explicit QuadTreePoint(const float2& position, T* userData)
            : x(position.x), y(position.y), userData(userData)
        {
        }
    };

    struct QuadTreeBoundary
    {
        float left;
        float top;
        float width;
        float height;

        explicit QuadTreeBoundary(float x, float y, float width, float height)
            : left(x), top(y), width(width), height(height)
        {
        }

        bool contains(float px, float py) const
        {
            return (px >= left && px <= left + width && py >= top && py <= top + height);
        }
    };

    template <typename T>
    concept QuadTreeQueryShape = requires(const T& shape, const QuadTreePoint<T>& point, const QuadTreeBoundary& boundary) {
        { shape.contains(point) } -> std::convertible_to<bool>;
        { shape.intersects(boundary) } -> std::convertible_to<bool>;
    };

    struct QuadTreeQueryRect
    {
        float x;
        float y;
        float width;
        float height;

        explicit QuadTreeQueryRect(const float2& position, const float2& size)
            : x(position.x), y(position.y), width(size.x), height(size.y)
        {
        }

        template <typename T>
        bool contains(const QuadTreePoint<T>& point) const
        {
            return (point.x >= x && point.x <= x + width && point.y >= y && point.y <= y + height);
        }

        bool intersects(const QuadTreeBoundary& boundary) const
        {
            return not(boundary.left > x + width || boundary.left + boundary.width < x || boundary.top > y + height || boundary.top + boundary.height < y);
        }
    };

    struct QuadTreeQueryCircle
    {
        float x;
        float y;
        float radius;

        explicit QuadTreeQueryCircle(const float2& position, float radius)
            : x(position.x), y(position.y), radius(radius)
        {
        }

        template <typename T>
        bool contains(const QuadTreePoint<T>& point) const
        {
            const float dx = point.x - x;
            const float dy = point.y - y;
            return (dx * dx + dy * dy) <= (radius * radius);
        }

        bool intersects(const QuadTreeBoundary& boundary) const
        {
            const float closestX = std::clamp(x, boundary.left, boundary.left + boundary.width);
            const float closestY = std::clamp(y, boundary.top, boundary.top + boundary.height);
            const float dx = x - closestX;
            const float dy = y - closestY;
            return (dx * dx + dy * dy) <= (radius * radius);
        }
    };

    template <typename T>
    class QuadTree
    {
    public:
        explicit QuadTree(const QuadTreeBoundary& boundary, size_t capacity)
            : boundary(boundary), isSubdivided(false), capacity(capacity)
        {
        }

        void reset()
        {
            points.clear();
            isSubdivided = false;
            topLeft.reset();
            topRight.reset();
            bottomRight.reset();
            bottomLeft.reset();
        }

        std::vector<QuadTreePoint<T>> query(const QuadTreeQueryShape auto& shape) const
        {
            // Check if the current node's boundary intersects with the query shape
            if (not shape.intersects(boundary)) {
                return {}; // Return empty if no intersection
            }

            std::vector<QuadTreePoint<T>> foundPoints;

            for (const auto& point : points) {
                if (shape.contains(point)) {
                    foundPoints.push_back(point);
                }
            }

            if (isSubdivided) {
                const auto topLeftPoints = topLeft->query(shape);
                const auto topRightPoints = topRight->query(shape);
                const auto bottomRightPoints = bottomRight->query(shape);
                const auto bottomLeftPoints = bottomLeft->query(shape);

                foundPoints.insert(foundPoints.end(), topLeftPoints.begin(), topLeftPoints.end());
                foundPoints.insert(foundPoints.end(), topRightPoints.begin(), topRightPoints.end());
                foundPoints.insert(foundPoints.end(), bottomRightPoints.begin(), bottomRightPoints.end());
                foundPoints.insert(foundPoints.end(), bottomLeftPoints.begin(), bottomLeftPoints.end());
            }

            return foundPoints;
        }

        bool insert(const QuadTreePoint<T>& point)
        {
            if (not boundary.contains(point.x, point.y)) {
                return false; // Point is out of bounds
            }

            const bool hasCapacity = points.size() < capacity;
            if (hasCapacity) {
                points.push_back(point);
                return true;
            }

            if (not isSubdivided) {
                subdivide();
            }

            if (topLeft->insert(point)) return true;
            if (topRight->insert(point)) return true;
            if (bottomRight->insert(point)) return true;
            if (bottomLeft->insert(point)) return true;

            return false;
        }

        void show() const
        {
            // Draw the current node's boundary
            noFill();
            stroke(255, 50);
            strokeWeight(1.0f);
            rect(boundary.left, boundary.top, boundary.width, boundary.height);

            if (isSubdivided) {
                topLeft->show();
                topRight->show();
                bottomRight->show();
                bottomLeft->show();
            }
        }

    private:
        void subdivide()
        {
            // Subdivide the current node into four quadrants
            isSubdivided = true;
            topLeft = std::make_unique<QuadTree<T>>(QuadTreeBoundary {boundary.left, boundary.top, boundary.width * 0.5f, boundary.height * 0.5f}, capacity);
            topRight = std::make_unique<QuadTree<T>>(QuadTreeBoundary {boundary.left + boundary.width * 0.5f, boundary.top, boundary.width * 0.5f, boundary.height * 0.5f}, capacity);
            bottomRight = std::make_unique<QuadTree<T>>(QuadTreeBoundary {boundary.left + boundary.width * 0.5f, boundary.top + boundary.height * 0.5f, boundary.width * 0.5f, boundary.height * 0.5f}, capacity);
            bottomLeft = std::make_unique<QuadTree<T>>(QuadTreeBoundary {boundary.left, boundary.top + boundary.height * 0.5f, boundary.width * 0.5f, boundary.height * 0.5f}, capacity);
        }

        std::unique_ptr<QuadTree<T>> topLeft;
        std::unique_ptr<QuadTree<T>> topRight;
        std::unique_ptr<QuadTree<T>> bottomRight;
        std::unique_ptr<QuadTree<T>> bottomLeft;
        bool isSubdivided;
        size_t capacity;

        QuadTreeBoundary boundary;
        std::vector<QuadTreePoint<T>> points;
    };

    struct Particle
    {
        float2 position;
        float2 velocity;
        color_t color;
        float lifetimeInSeconds;
        float initialLifetimeInSeconds;

        explicit Particle(const float2& position, const float2& velocity, float lifetimeInSeconds)
            : position(position), velocity(velocity), color(s_colorizer->getColor(position, velocity)), lifetimeInSeconds(lifetimeInSeconds), initialLifetimeInSeconds(lifetimeInSeconds)
        {
        }

        bool isAlive() const
        {
            return lifetimeInSeconds > 0.0f;
        }

        float getAlpha() const
        {
            return std::max(lifetimeInSeconds / initialLifetimeInSeconds, 0.0f);
        }

        void update(float deltaTime)
        {
            lifetimeInSeconds -= deltaTime;
            position += velocity * deltaTime;
        }

        void show() const
        {
            const color_t fillColor = withAlpha(color, static_cast<int>(getAlpha() * 255.0f));

            noStroke();
            fill(fillColor);
            ellipse(position.x, position.y, 12.0f, 12.0f);
        }
    };

    class ParticleMovement
    {
    };

    struct ParticleSystem
    {
        std::vector<Particle> particles;
        QuadTree<Particle> quadTree;

        explicit ParticleSystem(size_t particleCount)
            : quadTree(QuadTreeBoundary {0.0f, 0.0f, (float)getPhysicalWidth(), (float)getPhysicalHeight()}, 4)
        {
            spawnParticles(
                []() {
                    const auto width = getPhysicalWidth();
                    const auto height = getPhysicalHeight();
                    const float positionX = randomFloat(static_cast<float>(width));
                    const float positionY = randomFloat(static_cast<float>(height));
                    return float2 {positionX, positionY};
                },
                particleCount
            );
        }

        void spawnParticles(float2 (*getPosition)(), size_t count)
        {
            particles.reserve(particles.size() + count);
            for (size_t i = 0; i < count; ++i) {
                const float2 randomDirection = float2::randomUnit();
                const float2 velocity = randomDirection * randomFloat(10.0f, 30.0f);
                const float lifetimeInSeconds = randomFloat(1.0f, 15.0f);

                particles.emplace_back(getPosition(), velocity, lifetimeInSeconds);
            }
        }

        void update(float deltaTime)
        {
            quadTree.reset();

            for (Particle& particle : particles) {
                particle.update(deltaTime);

                if (isOutOfSight(particle.position)) {
                    const auto [width, height] = getCanvasSize();
                    particle.position.x = std::clamp(particle.position.x, 0.0f, static_cast<float>(width));
                    particle.position.y = std::clamp(particle.position.y, 0.0f, static_cast<float>(height));
                    particle.velocity = float2::randomUnit() * randomFloat(50.0f, 150.0f);
                }
            }

            std::erase_if(particles, [](const Particle& particle) {
                return not particle.isAlive();
            });

            quadTree.reset();
            for (Particle& particle : particles) {
                quadTree.insert(QuadTreePoint<Particle>(particle.position, &particle));
            }

            // Repell from nearby particles using QuadTree
            for (Particle& particle : particles) {
                const float2 queryPosition = particle.position;
                const float queryRadius = 50.0f;
                const QuadTreeQueryCircle queryCircle(queryPosition, queryRadius);

                // noFill();
                // stroke(255, 50);
                // circle(queryPosition.x, queryPosition.y, queryRadius * 2.0f);

                const auto nearbyPoints = quadTree.query(queryCircle);
                for (const auto& point : nearbyPoints) {
                    Particle* otherParticle = point.userData;
                    if (otherParticle == &particle) continue;

                    const float2 direction = particle.position - otherParticle->position;
                    const float distanceSquared = lengthSquared(direction);
                    const float minDistance = 50.0f;
                    const float minDistanceSquared = minDistance * minDistance;

                    if (distanceSquared < minDistanceSquared && distanceSquared > 0.0f) {
                        const float distance = std::sqrt(distanceSquared);
                        const float2 repulsionForce = direction / distance * (minDistance - distance) * 5.0f;
                        particle.velocity += repulsionForce * deltaTime;
                        otherParticle->velocity -= repulsionForce * deltaTime;
                    }
                }
            }
        }

        void show() const
        {
            // These connection segments are stroke-only (endShape(ShapeType::lines, ...)
            // doesn't support fill) - particle.show() below re-enables fill for the dots.
            noFill();

            // Check for nearby particles using QuadTree
            for (const Particle& particle : particles) {
                const float2 queryPosition = particle.position;
                const float queryRadius = 100.0f;
                const QuadTreeQueryCircle queryCircle(queryPosition, queryRadius);

                const auto nearbyPoints = quadTree.query(queryCircle);
                for (const auto& point : nearbyPoints) {
                    Particle* otherParticle = point.userData;
                    if (otherParticle == &particle) continue;

                    const float distance = length(particle.position - otherParticle->position);
                    if (distance > queryRadius) continue;

                    const float alpha = 1.0f - (distance / queryRadius);
                    const float particleAlpha = particle.getAlpha() * 255.0f * alpha;
                    const float otherParticleAlpha = otherParticle->getAlpha() * 255.0f * alpha;

                    beginShape();
                    stroke(withAlpha(particle.color, particleAlpha));
                    vertex(particle.position.x, particle.position.y);
                    stroke(withAlpha(otherParticle->color, otherParticleAlpha));
                    vertex(otherParticle->position.x, otherParticle->position.y);
                    endShape(ShapeType::lines, false);
                }
            }

            for (const auto& particle : particles) {
                particle.show();
            }

            quadTree.show();
        }

        bool isOutOfSight(const float2& position) const
        {
            const auto [width, height] = getCanvasSize();
            return position.x < 0.0f || position.x > static_cast<float>(width) || position.y < 0.0f || position.y > static_cast<float>(height);
        }
    };

    inline static constexpr const char* pixelateSource = R"(
        uniform float u_BlockSize;

        vec4 effect(vec2 uv, vec2 texelSize, sampler2D tex) {
            vec2 block = texelSize * max(u_BlockSize, 1.0);
            vec2 blockUV = (floor(uv / block) + 0.5) * block;
            return texture(tex, blockUV);
        }
    )";

    inline constexpr const char* vignetteSource = R"(
        uniform float u_Strength;

        vec4 effect(vec2 uv, vec2 texelSize, sampler2D tex) {
            vec4 c = texture(tex, uv);
            vec2 centered = uv - 0.5;
            float vignette = 1.0 - dot(centered, centered) * u_Strength;
            return vec4(c.rgb * clamp(vignette, 0.0, 1.0), c.a);
        }
    )";

    struct ConnectedParticleSketch : Sketch
    {
        static constexpr int W = 1280;
        static constexpr int H = 720;

        std::unique_ptr<ParticleSystem> particleSystem;
        Shader pixelateShader;
        Shader vignetteShader;
        Framebuffer scene;
        bool isMouseDown;

        void setup() override
        {
            setWindowSize(W, H);
            setWindowTitle("Connected Particles");
            frameRate(60);

            scene = createFramebuffer(W, H);
            pixelateShader = loadEffectShader(pixelateSource);
            vignetteShader = loadEffectShader(vignetteSource);

            // s_colorizer = std::make_unique<GlobalHueColorizer>();
            s_colorizer = std::make_unique<GlobalHueColorizer>();
            particleSystem = std::make_unique<ParticleSystem>(50);
            isMouseDown = false;
        }

        void event(const WindowEvent& event) override
        {
            if (event.type == EventType::mousePress and event.mouseButton.button == MouseButton::left) {
                isMouseDown = true;
            } else if (event.type == EventType::mouseRelease and event.mouseButton.button == MouseButton::left) {
                isMouseDown = false;
            }
        }

        void draw() override
        {
            s_colorizer->update(getDeltaTime());

            const int dragThreshold = 2;
            const int mouseDragX = getMouseX() - getPMouseX();
            const int mouseDragY = getMouseY() - getPMouseY();
            const bool isMouseDragging = (std::abs(mouseDragX) > dragThreshold) || (std::abs(mouseDragY) > dragThreshold);

            if (isMouseDown and isMouseDragging) {
                const int newParticlesCount = std::sqrt(mouseDragX * mouseDragX + mouseDragY * mouseDragY) / 10;

                particleSystem->spawnParticles(
                    []() {
                        const int mx = getMouseX();
                        const int my = getMouseY();
                        return float2 {static_cast<float>(mx), static_cast<float>(my)};
                    },
                    newParticlesCount
                );
            }

            // The particle system is rendered into its own Framebuffer rather than
            // straight onto the window canvas, since the vignette shader pass needs to
            // read "everything drawn so far" as a texture.
            pushCanvas(scene);
            {
                background(21, 75);
                blendMode(BlendMode::additive);
                particleSystem->update(getDeltaTime());
                particleSystem->show();
            }
            popCanvas();

            shader(vignetteShader);
            setUniform(vignetteShader, "u_Strength", uniform(2.5f));
            image(scene.colorTexture, 0, 0, W, H);
            noShader();

            // Pixelate instead, as an alternative:
            // shader(pixelateShader);
            // setUniform(pixelateShader, "u_BlockSize", uniform(3.5f));
            // setUniform(pixelateShader, "u_TexelSize", uniform(1.0f / W, 1.0f / H));
            // image(scene.colorTexture, 0, 0, W, H);
            // noShader();

            blendMode(BlendMode::alpha);
            fill(255);
            textSize(32.0f);
            text("Frame Rate: " + std::to_string(static_cast<int>(getFrameRate())), 20.0f, 40.0f);
        }

        void destroy() override
        {
            unload(pixelateShader);
            unload(vignetteShader);
            unload(scene);
        }
    };
} // namespace

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<ConnectedParticleSketch>();
    }
} // namespace p5cpp
