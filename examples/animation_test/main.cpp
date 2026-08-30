#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

#include <concepts>
#include <ranges>

using namespace p5;

using Curve = float (*)(float);

struct Curves
{
    inline static constexpr Curve linear = p5::animation::easeLinear;
};

struct wait_for_sequence_entry
{
    float durationInSeconds;
};

inline static constexpr wait_for_sequence_entry waitFor(float durationInSeconds)
{
    return wait_for_sequence_entry {
        .durationInSeconds = durationInSeconds,
    };
}

struct snap_to_sequence_entry
{
    float2 position;
};

inline static constexpr snap_to_sequence_entry snapTo(float2 position)
{
    return snap_to_sequence_entry {
        .position = position,
    };
}

struct tween_to_sequence_entry
{
    float2 position;
    float durationInSeconds;
    Curve curve;
};

inline static constexpr tween_to_sequence_entry tweenTo(float2 position, float durationInSeconds, Curve curve)
{
    return tween_to_sequence_entry {
        .position = position,
        .durationInSeconds = durationInSeconds,
        .curve = curve,
    };
}

using sequence_entry = std::variant<wait_for_sequence_entry, snap_to_sequence_entry, tween_to_sequence_entry>;

inline static constexpr float getDurationInSeconds(const snap_to_sequence_entry& entry) { return 0.0f; }
inline static constexpr float getDurationInSeconds(const tween_to_sequence_entry& entry) { return entry.durationInSeconds; }
inline static constexpr float getDurationInSeconds(const wait_for_sequence_entry& entry) { return entry.durationInSeconds; }
inline static constexpr float getDurationInSeconds(const sequence_entry& entry)
{
    return std::visit(
        [](const auto& e) {
            return getDurationInSeconds(e);
        },
        entry
    );
}

struct sequence
{
    std::vector<sequence_entry> entries;
    float elapsedTimeInSeconds;
    float2 currentPosition;
};

inline static constexpr sequence createSequence(std::ranges::range auto&& elements)
    requires requires {
        std::is_same_v<std::ranges::range_value_t<decltype(elements)>, sequence_entry>;
    }
{
    return sequence {
        .entries = {std::ranges::begin(elements), std::ranges::end(elements)},
        .elapsedTimeInSeconds = 0.0f,
        .currentPosition = {0.0f, 0.0f},
    };
}

inline static void advance_entry(sequence& sequence, snap_to_sequence_entry& entry, float deltaTime) { entry.position = entry.position; }
inline static void advance_entry(sequence& sequence, tween_to_sequence_entry& entry, float deltaTime)
{
    const size_t currentEntryIndex = getCurrentEntryIndex(sequence);
    const float currentEntryElapsedTimeInSeconds = getCurrentEntryElapsedTimeInSeconds(sequence);
    const float progress = currentEntryElapsedTimeInSeconds / entry.durationInSeconds;
    const float easedProgress = entry.curve(progress);

    if (currentEntryIndex > 0) {
        const auto& previousEntry = std::get<sequence_entry>(sequence.entries[currentEntryIndex - 1]);
        if (std::holds_alternative<snap_to_sequence_entry>(previousEntry)) {
            const auto& previousSnapToEntry = std::get<snap_to_sequence_entry>(previousEntry);
            sequence.currentPosition = lerp(previousSnapToEntry.position, entry.position, easedProgress);
        } else if (std::holds_alternative<tween_to_sequence_entry>(previousEntry)) {
            const auto& previousTweenToEntry = std::get<tween_to_sequence_entry>(previousEntry);
            sequence.currentPosition = lerp(previousTweenToEntry.position, entry.position, easedProgress);
        }
    } else {
        sequence.currentPosition = lerp(float2{0.0f, 0.0f}, entry.position, easedProgress);
    }
}

inline static size_t getCurrentEntryIndex(const sequence& sequence)
{
    float elapsedTimeInSeconds = 0.0f;
    for (size_t i = 0; i < sequence.entries.size(); ++i) {
        const auto& entry = sequence.entries[i];
        elapsedTimeInSeconds += getDurationInSeconds(entry);
        if (sequence.elapsedTimeInSeconds < elapsedTimeInSeconds) {
            return i;
        }
    }

    return sequence.entries.size();
}

inline static float getCurrentEntryElapsedTimeInSeconds(const sequence& sequence)
{
    float elapsedTimeInSeconds = 0.0f;
    for (size_t i = 0; i < sequence.entries.size(); ++i) {
        const auto& entry = sequence.entries[i];
        const float entryDurationInSeconds = getDurationInSeconds(entry);
        if (sequence.elapsedTimeInSeconds < elapsedTimeInSeconds + entryDurationInSeconds) {
            return sequence.elapsedTimeInSeconds - elapsedTimeInSeconds;
        }
        elapsedTimeInSeconds += entryDurationInSeconds;
    }

    return 0.0f;
}

inline static void advance(sequence& sequence, float deltaTime)
{
    const size_t currentEntryIndex = getCurrentEntryIndex(sequence);
    sequence_entry& currentEntry = sequence.entries[currentEntryIndex];

    advance_entry(currentEntry, deltaTime);

    // const float currentEntryDurationInSeconds = getDurationInSeconds(sequence.entries[currentEntryIndex]);
    // const float currentEntryElapsedTimeInSeconds = getCurrentEntryElapsedTimeInSeconds(sequence);
    // const float progress = currentEntryElapsedTimeInSeconds / currentEntryDurationInSeconds;
    // const bool isCurrentEntryComplete = progress >= 1.0f;
}

struct AnimationTest : Sketch
{
    sequence seq = createSequence(std::vector<sequence_entry> {
        snapTo({.x = 200.0f, .y = 200.0f}),
        tweenTo({.x = 400.0f, .y = 200.0f}, 1.25f, Curves::linear),
        waitFor(0.5f),
        tweenTo({.x = 400.0f, .y = 400.0f}, 1.25f, Curves::linear),
    });

    void setup() override
    {
        setWindowSize(1280, 720);
    }

    void draw() override
    {
        background(rgba(31, 31, 51));
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
