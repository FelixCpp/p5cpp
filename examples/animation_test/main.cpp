#include <cassert>
#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

using namespace p5;
using namespace p5::animation;

using Curve = float (*)(float);

enum class PlaybackBehavior
{
    waiting,
    animating,
};

struct advance_result
{
    float timeConsumed;
    float2 position;
    bool isCompleted;
};

struct sequence_entry
{
    virtual ~sequence_entry() = default;
    virtual float2 getEndPosition() const = 0;
    virtual advance_result advance(float deltaTime) = 0;
    virtual PlaybackBehavior getBehavior() const = 0;
};

struct sequence_entry_builder_options
{
    sequence_entry* previousEntry;
};

struct sequence_entry_builder
{
    virtual ~sequence_entry_builder() = default;
    virtual std::unique_ptr<sequence_entry> build(const sequence_entry_builder_options& options) = 0;
};

struct snap_to_sequence_entry : sequence_entry
{
    float2 position;

    constexpr explicit snap_to_sequence_entry(float2 position)
        : position(position)
    {
    }

    float2 getEndPosition() const override
    {
        return position;
    }

    advance_result advance(float deltaTime) override
    {
        return {
            .timeConsumed = 0.0f,
            .position = position,
            .isCompleted = true,
        };
    }

    PlaybackBehavior getBehavior() const override
    {
        return PlaybackBehavior::animating;
    }
};

struct snap_to_sequence_entry_builder : sequence_entry_builder
{
    float2 position;

    constexpr explicit snap_to_sequence_entry_builder(float2 position) : position(position) {}

    std::unique_ptr<sequence_entry> build(const sequence_entry_builder_options& options) override
    {
        return std::make_unique<snap_to_sequence_entry>(position);
    }
};

struct tween_to_sequence_entry : sequence_entry
{
    float2 start;
    float2 end;
    float durationInSeconds;
    Curve curve;
    float elapsedTimeInSeconds;

    constexpr explicit tween_to_sequence_entry(float2 start, float2 end, float durationInSeconds, Curve curve)
        : start(start),
          end(end),
          durationInSeconds(durationInSeconds),
          curve(curve),
          elapsedTimeInSeconds(0.0f)
    {
    }

    float2 getEndPosition() const override
    {
        return end;
    }

    advance_result advance(float deltaTime) override
    {
        const float timeRemaining = durationInSeconds - elapsedTimeInSeconds;
        const float timeToConsume = std::min(deltaTime, timeRemaining);
        const bool isCompleted = elapsedTimeInSeconds + deltaTime >= durationInSeconds;
        elapsedTimeInSeconds = std::min(elapsedTimeInSeconds + timeToConsume, durationInSeconds);

        const float progress = curve(durationInSeconds > 0.0f ? (elapsedTimeInSeconds / durationInSeconds) : 1.0f);
        const float dx = start.x + (end.x - start.x) * progress;
        const float dy = start.y + (end.y - start.y) * progress;

        return {
            .timeConsumed = timeToConsume,
            .position = {.x = dx, .y = dy},
            .isCompleted = isCompleted,
        };
    }

    PlaybackBehavior getBehavior() const override
    {
        return PlaybackBehavior::animating;
    }
};

inline static std::unique_ptr<sequence_entry_builder> snapTo(float2 position)
{
    return std::make_unique<snap_to_sequence_entry_builder>(position);
}

struct tween_to_sequence_entry_builder : sequence_entry_builder
{
    float2 destination;
    float durationInSeconds;
    Curve curve;

    constexpr explicit tween_to_sequence_entry_builder(float2 end, float durationInSeconds, Curve curve)
        : destination(end),
          durationInSeconds(durationInSeconds),
          curve(curve)
    {
    }

    std::unique_ptr<sequence_entry> build(const sequence_entry_builder_options& options) override
    {
        assert(options.previousEntry != nullptr and "tween_to_sequence_entry_builder requires a previous entry to determine the start position.");

        return std::make_unique<tween_to_sequence_entry>(
            options.previousEntry->getEndPosition(),
            destination,
            durationInSeconds,
            curve
        );
    }
};

inline static std::unique_ptr<sequence_entry_builder> tweenTo(float2 end, float durationInSeconds, Curve curve)
{
    return std::make_unique<tween_to_sequence_entry_builder>(end, durationInSeconds, curve);
}

struct wait_for_sequence_entry : sequence_entry
{
    float durationInSeconds;
    float elapsedTimeInSeconds;

    sequence_entry* previous;

    constexpr explicit wait_for_sequence_entry(float durationInSeconds, sequence_entry* previous)
        : durationInSeconds(durationInSeconds),
          elapsedTimeInSeconds(0.0f),
          previous(previous)
    {
    }

    float2 getEndPosition() const override
    {
        return previous->getEndPosition();
    }

    advance_result advance(float deltaTime) override
    {
        const float timeRemaining = durationInSeconds - elapsedTimeInSeconds;
        const float timeToConsume = std::min(deltaTime, timeRemaining);
        const bool isCompleted = elapsedTimeInSeconds + deltaTime >= durationInSeconds;

        elapsedTimeInSeconds = std::min(elapsedTimeInSeconds + timeToConsume, durationInSeconds);

        return {
            .timeConsumed = timeToConsume,
            .position = previous->getEndPosition(),
            .isCompleted = isCompleted,
        };
    }

    PlaybackBehavior getBehavior() const override
    {
        return PlaybackBehavior::waiting;
    }
};

struct wait_for_sequence_entry_builder : sequence_entry_builder
{
    float durationInSeconds;

    constexpr explicit wait_for_sequence_entry_builder(float durationInSeconds)
        : durationInSeconds(durationInSeconds)
    {
    }

    std::unique_ptr<sequence_entry> build(const sequence_entry_builder_options& options) override
    {
        return std::make_unique<wait_for_sequence_entry>(durationInSeconds, options.previousEntry);
    }
};

inline static std::unique_ptr<sequence_entry_builder> waitFor(float durationInSeconds)
{
    return std::make_unique<wait_for_sequence_entry_builder>(durationInSeconds);
}

struct sequence
{
    std::vector<std::unique_ptr<sequence_entry>> entries;
    size_t currentIndex;
    float2 position;
    bool completed;

    explicit sequence(std::vector<std::unique_ptr<sequence_entry>> entries)
        : entries(std::move(entries)),
          currentIndex(0),
          position(),
          completed(false)
    {
    }

    void advance(float deltaTime)
    {
        while (deltaTime > 0.0f and not completed) {
            const std::unique_ptr<sequence_entry>& currentEntry = entries[currentIndex];
            const advance_result result = currentEntry->advance(deltaTime);
            position = result.position;
            deltaTime -= result.timeConsumed;

            if (result.isCompleted) {
                if (currentIndex + 1 < entries.size()) {
                    ++currentIndex;
                } else {
                    completed = true;
                }
            }
        }
    }

    float2 getPosition() const
    {
        return position;
    }

    PlaybackBehavior getPlaybackBehavior() const
    {
        if (completed) {
            return PlaybackBehavior::waiting;
        }

        return entries[currentIndex]->getBehavior();
    }
};

inline static sequence createSequence(const std::vector<std::unique_ptr<sequence_entry_builder>>& builders)
{
    assert(not builders.empty() and "createSequence requires at least one sequence_entry_builder.");

    std::vector<std::unique_ptr<sequence_entry>> entries;
    sequence_entry* previousEntry = nullptr;
    for (size_t i = 0; i < builders.size(); ++i) {
        const std::unique_ptr<sequence_entry_builder>& builder = builders[i];

        const sequence_entry_builder_options options = {
            .previousEntry = previousEntry,
        };

        std::unique_ptr<sequence_entry> entry = builder->build(options);
        auto& instance = entries.emplace_back(std::move(entry));
        previousEntry = instance.get();
    }

    return sequence(std::move(entries));
}

struct AnimationTest : Sketch
{
    static std::vector<std::unique_ptr<sequence_entry_builder>> getSequence()
    {
        std::vector<std::unique_ptr<sequence_entry_builder>> entries;

        entries.push_back(snapTo({.x = 200.0f, .y = 200.0f}));
        entries.push_back(tweenTo({.x = 400.0f, .y = 200.0f}, 0.75f, easeInOutSine));
        entries.push_back(waitFor(0.5f));
        entries.push_back(tweenTo({.x = 400.0f, .y = 400.0f}, 0.5f, easeInOutBack));
        entries.push_back(tweenTo({.x = 200.0f, .y = 400.0f}, 3.0f, easeInOutBounce));
        entries.push_back(waitFor(2.0f));
        entries.push_back(tweenTo({.x = 300.0f, .y = 300.0f}, 1.8f, easeInOutExpo));
        entries.push_back(waitFor(0.5f));
        entries.push_back(snapTo({.x = 200.0f, .y = 200.0f}));

        return entries;
    }

    sequence seq = createSequence(getSequence());

    void setup() override
    {
        setWindowSize(1280, 720);
    }

    void draw() override
    {
        seq.advance(getDeltaTime());

        background(rgba(31, 31, 51));

        const float2 ballPosition = seq.getPosition();

        if (seq.getPlaybackBehavior() == PlaybackBehavior::animating) {
            fill(rgba(255));
        } else {
            fill(rgba(255, 0, 0));
        }
        noStroke();
        circle(ballPosition.x, ballPosition.y, 50.0f);
    }
};

SketchSpec p5::createSpec()
{
    return {
        .sketch = [] {
            return std::make_unique<AnimationTest>();
        }
    };
}
