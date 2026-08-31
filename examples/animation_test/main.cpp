#include <cassert>
#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

using namespace p5;
using namespace p5::animation;

using Curve = float (*)(float);
using ProgressTransformer = std::function<void(float)>;

enum class PlaybackBehavior
{
    animating,
    waiting,
};

struct advance_result
{
    float timeConsumed;
    bool isCompleted;
};

struct transition
{
    virtual ~transition() = default;
    virtual advance_result advance(float deltaTimeInSeconds) = 0;
    virtual PlaybackBehavior getPlaybackBehavior() const = 0;
};

struct tween_transition : transition
{
    Curve curve;
    float durationInSeconds;
    float elapsedTimeInSeconds;

    ProgressTransformer transformer;

    constexpr explicit tween_transition(float duration, Curve curve, ProgressTransformer transformer)
        : curve(curve),
          durationInSeconds(duration),
          elapsedTimeInSeconds(0.0f),
          transformer(std::move(transformer))
    {
    }

    advance_result advance(float deltaTimeInSeconds) override
    {
        const float timeRemaining = durationInSeconds - elapsedTimeInSeconds;
        const float timeToConsume = std::min(deltaTimeInSeconds, timeRemaining);
        const bool isCompleted = (elapsedTimeInSeconds + deltaTimeInSeconds) >= durationInSeconds;

        elapsedTimeInSeconds = std::min(elapsedTimeInSeconds + deltaTimeInSeconds, durationInSeconds);

        const float progress = curve(durationInSeconds > 0.0f ? (elapsedTimeInSeconds / durationInSeconds) : 1.0f);
        transformer(progress);

        return {
            .timeConsumed = timeToConsume,
            .isCompleted = isCompleted
        };
    }

    PlaybackBehavior getPlaybackBehavior() const override
    {
        return PlaybackBehavior::animating;
    }
};

struct tween_transition_builder
{
    float durationInSeconds;
    Curve curve;
    ProgressTransformer transformer;

    constexpr explicit tween_transition_builder(float duration, Curve curve, ProgressTransformer transformer)
        : durationInSeconds(duration),
          curve(curve),
          transformer(std::move(transformer))
    {
    }

    std::unique_ptr<transition> build() const
    {
        return std::make_unique<tween_transition>(durationInSeconds, curve, transformer);
    }
};

inline static constexpr tween_transition_builder tween(float duration, Curve curve, ProgressTransformer transformer)
{
    return tween_transition_builder {duration, curve, std::move(transformer)};
}

struct wait_for_transition : transition
{
    float durationInSeconds;
    float elapsedTimeInSeconds;

    constexpr explicit wait_for_transition(float duration)
        : durationInSeconds(duration),
          elapsedTimeInSeconds(0.0f)
    {
    }

    advance_result advance(float deltaTimeInSeconds) override
    {
        const float timeRemaining = durationInSeconds - elapsedTimeInSeconds;
        const float timeToConsume = std::min(deltaTimeInSeconds, timeRemaining);
        const bool isCompleted = (elapsedTimeInSeconds + deltaTimeInSeconds) >= durationInSeconds;

        elapsedTimeInSeconds = std::min(elapsedTimeInSeconds + deltaTimeInSeconds, durationInSeconds);

        return {
            .timeConsumed = timeToConsume,
            .isCompleted = isCompleted
        };
    }

    PlaybackBehavior getPlaybackBehavior() const override
    {
        return PlaybackBehavior::waiting;
    }
};

struct wait_for_transition_builder
{
    float durationInSeconds;

    constexpr explicit wait_for_transition_builder(float duration)
        : durationInSeconds(duration)
    {
    }

    std::unique_ptr<transition> build() const
    {
        return std::make_unique<wait_for_transition>(durationInSeconds);
    }
};

inline static constexpr wait_for_transition_builder wait_for(float duration)
{
    return wait_for_transition_builder {duration};
}

struct transition_chain : transition
{
    virtual ~transition_chain() = default;
};

struct sequential_transition_chain : transition_chain
{
    std::vector<std::unique_ptr<transition>> transitions;
    size_t currentTransitionIndex;
    bool completed;

    explicit sequential_transition_chain(std::vector<std::unique_ptr<transition>> transitions)
        : transitions(std::move(transitions)),
          currentTransitionIndex(0),
          completed(false)
    {
    }

    advance_result advance(float deltaTimeInSeconds) override
    {
        float initialDeltaTime = deltaTimeInSeconds;
        while (deltaTimeInSeconds > 0.0f and not completed) {
            const std::unique_ptr<transition>& currentTransition = transitions[currentTransitionIndex];
            const advance_result result = currentTransition->advance(deltaTimeInSeconds);
            deltaTimeInSeconds -= result.timeConsumed;

            if (result.isCompleted) {
                if (currentTransitionIndex + 1 < transitions.size()) {
                    currentTransitionIndex++;
                } else {
                    completed = true;
                }
            }
        }

        return {
            .timeConsumed = std::max(initialDeltaTime - deltaTimeInSeconds, 0.0f),
            .isCompleted = completed
        };
    }

    PlaybackBehavior getPlaybackBehavior() const override
    {
        if (completed) {
            return PlaybackBehavior::waiting;
        }

        const std::unique_ptr<transition>& currentTransition = transitions[currentTransitionIndex];
        return currentTransition->getPlaybackBehavior();
    }
};

// struct parallel_transition_chain : transition_chain
// {
//     std::vector<std::unique_ptr<transition>> transitions;
//     bool completed;
//
//     explicit parallel_transition_chain(std::vector<std::unique_ptr<transition>> transitions)
//         : transitions(std::move(transitions)),
//           completed(false)
//     {
//     }
//
//     advance_result advance(float deltaTimeInSeconds) override
//     {
//         float timeConsumed = 0.0f;
//         bool allCompleted = true;
//
//         for (const std::unique_ptr<transition>& transition : transitions) {
//             if (transition->getPlaybackBehavior() != PlaybackBehavior::waiting) {
//                 const advance_result result = transition->advance(deltaTimeInSeconds);
//                 timeConsumed = std::max(timeConsumed, result.timeConsumed);
//                 allCompleted &= result.isCompleted;
//             }
//         }
//
//         completed = allCompleted;
//
//         return {
//             .timeConsumed = timeConsumed,
//             .isCompleted = completed
//         };
//     }
//
//     PlaybackBehavior getPlaybackBehavior() const override
//     {
//         if (completed) {
//             return PlaybackBehavior::waiting;
//         }
//
//         for (const std::unique_ptr<transition>& transition : transitions) {
//             if (transition->getPlaybackBehavior() == PlaybackBehavior::animating) {
//                 return PlaybackBehavior::animating;
//             }
//         }
//
//         return PlaybackBehavior::waiting;
//     }
// };

template <typename TransitionBuilder>
concept transition_builder = requires(TransitionBuilder builder) {
    { builder.build() } -> std::same_as<std::unique_ptr<transition>>;
};

template <typename... TransitionBuilderRange>
    requires(transition_builder<TransitionBuilderRange> and ...)
std::unique_ptr<sequential_transition_chain> sequence(TransitionBuilderRange&&... transitions)
{
    std::vector<std::unique_ptr<transition>> transitionList;
    (transitionList.push_back(transitions.build()), ...);

    return std::make_unique<sequential_transition_chain>(std::move(transitionList));
}

struct AnimationTest : Sketch
{
    float x = 0.0f;
    float y = 200.0f;
    float scl = 1.0f;

    std::unique_ptr<transition_chain> chain = sequence(
        wait_for(1.0f),
        tween(1.5f, easeInOutSine, [this](float progress) {
            x = lerp(0.0f, 400.0f, progress);
        })
    );

    void setup() override
    {
        setWindowSize(1280, 720);
    }

    void draw() override
    {
        chain->advance(getDeltaTime());

        background(rgba(31, 31, 51));
        translate(x, y);
        scale(scl, scl);

        if (chain->getPlaybackBehavior() == PlaybackBehavior::waiting) {
            fill(rgba(255, 0, 0));
        } else {
            fill(rgba(255));
        }

        noStroke();
        circle(0.0f, 0.0f, 50.0f);
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
