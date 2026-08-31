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

struct tween_transition
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

    advance_result advance(float deltaTimeInSeconds)
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

    PlaybackBehavior getPlaybackBehavior() const
    {
        return PlaybackBehavior::animating;
    }
};

inline static constexpr tween_transition tween(float duration, Curve curve, ProgressTransformer transformer)
{
    return tween_transition {duration, curve, std::move(transformer)};
}

struct wait_for_transition
{
    float durationInSeconds;
    float elapsedTimeInSeconds;

    constexpr explicit wait_for_transition(float duration)
        : durationInSeconds(duration),
          elapsedTimeInSeconds(0.0f)
    {
    }

    advance_result advance(float deltaTimeInSeconds)
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

    PlaybackBehavior getPlaybackBehavior() const
    {
        return PlaybackBehavior::waiting;
    }
};

inline static constexpr wait_for_transition wait_for(float duration)
{
    return wait_for_transition {duration};
}

template <typename Transition>
concept transition = requires(Transition transition, float deltaTimeInSeconds) {
    { transition.advance(deltaTimeInSeconds) } -> std::same_as<advance_result>;
    { transition.getPlaybackBehavior() } -> std::same_as<PlaybackBehavior>;
};

struct generic_transition
{
    virtual ~generic_transition() = default;
    virtual advance_result advance(float deltaTimeInSeconds) = 0;
    virtual PlaybackBehavior getPlaybackBehavior() const = 0;
};

template <transition Transition> struct transition_wrapper : generic_transition
{
    Transition transition;

    inline explicit constexpr transition_wrapper(Transition transition)
        : transition(std::move(transition))
    {
    }

    inline advance_result advance(float deltaTimeInSeconds) override
    {
        return transition.advance(deltaTimeInSeconds);
    }

    inline PlaybackBehavior getPlaybackBehavior() const override
    {
        return transition.getPlaybackBehavior();
    }
};

struct transition_container
{
    std::vector<std::unique_ptr<generic_transition>> transitions;

    template <typename... Transitions>
        requires(transition<std::remove_cvref_t<Transitions>> && ...)
    inline explicit transition_container(Transitions&&... transitions)
    {
        (this->transitions.push_back(std::make_unique<transition_wrapper<Transitions>>(std::forward<Transitions>(transitions))), ...);
    }
};

struct sequential_transition_chain : private transition_container
{
    size_t currentTransitionIndex;
    bool completed;

    template <typename... Transitions>
        requires(transition<std::remove_cvref_t<Transitions>> && ...)
    explicit sequential_transition_chain(Transitions&&... transitions)
        : transition_container(std::forward<Transitions>(transitions)...),
          currentTransitionIndex(0),
          completed(false)
    {
    }

    advance_result advance(float deltaTimeInSeconds)
    {
        const float initialDeltaTime = deltaTimeInSeconds;

        while (not completed) {
            const std::unique_ptr<generic_transition>& currentTransition = transitions[currentTransitionIndex];
            const advance_result result = currentTransition->advance(deltaTimeInSeconds);
            deltaTimeInSeconds -= result.timeConsumed;

            if (not result.isCompleted) {
                break; // Early out if the transition is not completed, as we don't want to advance to the next transition yet.
            }

            if (currentTransitionIndex + 1 < transitions.size()) {
                currentTransitionIndex++;
            } else {
                completed = true;
            }
        }

        return {
            .timeConsumed = std::max(initialDeltaTime - deltaTimeInSeconds, 0.0f),
            .isCompleted = completed
        };
    }

    PlaybackBehavior getPlaybackBehavior() const
    {
        if (completed) {
            return PlaybackBehavior::waiting;
        }

        return transitions[currentTransitionIndex]->getPlaybackBehavior();
    }
};

template <typename... TransitionRange>
    requires(transition<std::remove_cvref_t<TransitionRange>> && ...)
inline static constexpr sequential_transition_chain sequence(TransitionRange&&... transitions)
{
    return sequential_transition_chain {
        std::forward<TransitionRange>(transitions)...
    };
}

struct parallel_transition_chain : private transition_container
{
    std::vector<bool> transitionsCompleted;

    template <transition... Transitions>
    explicit parallel_transition_chain(Transitions&&... transitions)
        : transition_container(std::forward<Transitions>(transitions)...),
          transitionsCompleted(sizeof...(Transitions), false)
    {
    }

    advance_result advance(float deltaTimeInSeconds)
    {
        float timeConsumed = 0.0f;

        for (size_t i = 0; i < transitions.size(); ++i) {
            if (transitionsCompleted.at(i)) {
                continue;
            }

            const advance_result result = transitions.at(i)->advance(deltaTimeInSeconds);
            timeConsumed = std::max(timeConsumed, result.timeConsumed);
            transitionsCompleted[i] = result.isCompleted;
        }

        return {
            .timeConsumed = timeConsumed,
            .isCompleted = isEveryTransitionCompleted()
        };
    }

    bool isEveryTransitionCompleted() const
    {
        return std::all_of(transitionsCompleted.begin(), transitionsCompleted.end(), [](bool completed) {
            return completed;
        });
    }

    PlaybackBehavior getPlaybackBehavior() const
    {
        if (isEveryTransitionCompleted()) {
            return PlaybackBehavior::waiting;
        }

        for (const std::unique_ptr<generic_transition>& transition : transitions) {
            if (transition->getPlaybackBehavior() == PlaybackBehavior::animating) {
                return PlaybackBehavior::animating;
            }
        }

        return PlaybackBehavior::waiting;
    }
};

template <typename... TransitionRange>
    requires(transition<std::remove_cvref_t<TransitionRange>> && ...)
inline static constexpr parallel_transition_chain parallel(TransitionRange&&... transitions)
{
    return parallel_transition_chain {
        std::forward<TransitionRange>(transitions)...
    };
}

struct call_transition
{
    std::function<void()> callback;

    explicit call_transition(std::function<void()> callback)
        : callback(std::move(callback))
    {
    }

    advance_result advance(float deltaTimeInSeconds)
    {
        callback();

        return {
            .timeConsumed = 0.0f,
            .isCompleted = true
        };
    }

    PlaybackBehavior getPlaybackBehavior() const
    {
        return PlaybackBehavior::animating;
    }
};

inline static call_transition call(std::function<void()> callback)
{
    return call_transition {std::move(callback)};
}

struct wait_until_transition
{
    std::function<bool()> condition;

    explicit wait_until_transition(std::function<bool()> condition)
        : condition(std::move(condition))
    {
    }

    advance_result advance(float deltaTimeInSeconds)
    {
        const bool isCompleted = condition();

        return {
            .timeConsumed = 0.0f,
            .isCompleted = isCompleted
        };
    }

    PlaybackBehavior getPlaybackBehavior() const
    {
        return PlaybackBehavior::waiting;
    }
};

inline static wait_until_transition wait_until(std::function<bool()> condition)
{
    return wait_until_transition {std::move(condition)};
}

struct AnimationTest : Sketch
{
    float x = 200.0f;
    float y = 200.0f;
    float scl = 1.0f;

    sequential_transition_chain chain = sequence(
        wait_for(1.0f),
        tween(2.0f, easeInOutCubic, [this](float progress) {
            x = lerp(200.0f, 400.0f, progress);
        }),
        call([this] {
            x = 300.0f;
        }),
        wait_until([this] {
            return isMouseButtonPressed(MouseButton::Left);
        }),
        std::invoke([this]() {
            return parallel(
                tween(2.0f, easeInOutCubic, [this](float progress) {
                    y = lerp(200.0f, 400.0f, progress);
                }),
                std::invoke([this] {
                    return sequence(
                        tween(1.0f, easeInOutBounce, [this](float progress) {
                            scl = lerp(1.0f, 2.0f, progress);
                        }),
                        tween(2.0f, easeInOutCubic, [this](float progress) {
                            scl = lerp(2.0f, 1.0f, progress);
                        })
                    );
                })
            );
        })
    );

    void setup() override
    {
        setWindowSize(1280, 720);
    }

    void draw() override
    {
        chain.advance(getDeltaTime());

        background(rgba(31, 31, 51));
        translate(x, y);
        scale(scl, scl);

        if (chain.getPlaybackBehavior() == PlaybackBehavior::waiting) {
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
