#pragma once

#include <p5cpp/p5cpp.hpp>

// easing.hpp

namespace p5::animation::curves
{
    float easeLinear(float x);
    float easeInQuad(float x);
    float easeOutQuad(float x);
    float easeInOutQuad(float x);
    float easeInCubic(float x);
    float easeOutCubic(float x);
    float easeInOutCubic(float x);
    float easeInQuart(float x);
    float easeOutQuart(float x);
    float easeInOutQuart(float x);
    float easeInQuint(float x);
    float easeOutQuint(float x);
    float easeInOutQuint(float x);
    float easeInSine(float x);
    float easeOutSine(float x);
    float easeInOutSine(float x);
    float easeInExpo(float x);
    float easeOutExpo(float x);
    float easeInOutExpo(float x);
    float easeInCirc(float x);
    float easeOutCirc(float x);
    float easeInOutCirc(float x);
    float easeInBack(float x);
    float easeOutBack(float x);
    float easeInOutBack(float x);
    float easeInElastic(float x);
    float easeOutElastic(float x);
    float easeInOutElastic(float x);
    float easeInBounce(float x);
    float easeOutBounce(float x);
    float easeInOutBounce(float x);
} // namespace p5::animation::curves

namespace p5::animation
{
    typedef std::function<float(float)> Curve;

    Curve reverse(Curve curve);
} // namespace p5::animation

namespace p5::animation
{
    struct advance_result
    {
        float timeConsumed;
        bool isCompleted;
    };
} // namespace p5::animation

namespace p5::animation
{
    enum class transition_state
    {
        animating, // currently producing a visible change this frame
        waiting,   // running, but passively (counting time / waiting on a condition)
        completed, // fully done - safe to discard
    };
}

namespace p5::animation
{
    template <typename Transition>
    concept transition = requires(Transition transition) {
        { transition.advance(std::declval<float>()) } -> std::same_as<advance_result>;
        { transition.get_transition_state() } -> std::same_as<transition_state>;
    };

    template <typename... Transitions>
    concept transitions = (transition<std::remove_cvref_t<Transitions>> && ...);

    typedef std::function<void()> callback_function;
    typedef std::function<bool()> condition_function;
} // namespace p5::animation

namespace p5::animation
{
    typedef std::function<void(float)> progress_transformer;

    class tween_transition
    {
    public:
        explicit tween_transition(float duration, Curve curve, progress_transformer transformer);

        advance_result advance(float deltaTimeInSeconds);
        transition_state get_transition_state() const;

    private:
        float durationInSeconds;
        float elapsedTimeInSeconds;
        Curve curve;
        progress_transformer transformer;
        bool isCompleted;
    };

    tween_transition tween(float duration, Curve curve, progress_transformer transformer);
} // namespace p5::animation

namespace p5::animation
{
    class wait_for_transition
    {
    public:
        explicit wait_for_transition(float duration);

        advance_result advance(float deltaTimeInSeconds);
        transition_state get_transition_state() const;

    private:
        float durationInSeconds;
        float elapsedTimeInSeconds;
        bool isCompleted;
    };

    wait_for_transition wait_for(float duration);
} // namespace p5::animation

namespace p5::animation
{
    class wait_until_transition
    {
    public:
        explicit wait_until_transition(condition_function condition);

        advance_result advance(float deltaTimeInSeconds);
        transition_state get_transition_state() const;

    private:
        condition_function condition;
        bool isCompleted;
    };

    wait_until_transition wait_until(condition_function condition);
} // namespace p5::animation

namespace p5::animation
{
    class call_transition
    {
    public:
        explicit call_transition(callback_function callback);

        advance_result advance(float deltaTimeInSeconds);
        transition_state get_transition_state() const;

    private:
        callback_function callback;
        bool isCompleted;
    };

    call_transition call(callback_function callback);
} // namespace p5::animation

namespace p5::animation
{
    template <typename Transition>
        requires transition<std::remove_cvref_t<Transition>>
    class speed_adjusted_transition
    {
    public:
        explicit speed_adjusted_transition(Transition transition, float multiplier);
        advance_result advance(float deltaTimeInSeconds);
        transition_state get_transition_state() const;

    private:
        Transition transition;
        float multiplier;
    };

    template <typename Transition>
        requires transition<std::remove_cvref_t<Transition>>
    speed_adjusted_transition<Transition> adjust_speed(Transition transition, float multiplier);
} // namespace p5::animation

namespace p5::animation
{
    template <typename Transition>
    concept transition_factory = std::is_invocable_v<Transition> && transition<std::remove_cvref_t<std::invoke_result_t<Transition>>>;

    template <typename Factory>
        requires transition_factory<Factory>
    class repeat_transition
    {
    public:
        explicit repeat_transition(Factory factory, std::optional<size_t> repeatCount);
        advance_result advance(float deltaTimeInSeconds);
        transition_state get_transition_state() const;

    private:
        Factory factory;
        std::invoke_result_t<Factory> currentTransition;
        std::optional<size_t> remainingCycles;
        bool isCompleted;
    };

    template <typename Factory>
        requires transition_factory<Factory>
    repeat_transition<Factory> repeat(Factory factory, std::optional<size_t> repeatCount);
} // namespace p5::animation

namespace p5::animation
{
    struct generic_transition
    {
        virtual ~generic_transition() = default;
        virtual advance_result advance(float deltaTimeInSeconds) = 0;
        virtual transition_state get_transition_state() const = 0;
    };
} // namespace p5::animation

namespace p5::animation
{
    template <typename Transition>
        requires transition<Transition>
    class transition_wrapper : public generic_transition
    {
    public:
        explicit transition_wrapper(Transition transition);
        advance_result advance(float deltaTimeInSeconds) override;
        transition_state get_transition_state() const override;

    private:
        Transition transition;
    };
} // namespace p5::animation

namespace p5::animation
{
    class sequential_transition_chain
    {
    public:
        template <typename... Transitions>
            requires(transitions<Transitions...>)
        explicit sequential_transition_chain(Transitions... transitions);

        advance_result advance(float deltaTimeInSeconds);
        transition_state get_transition_state() const;

    private:
        std::vector<std::unique_ptr<generic_transition>> transitions;
        size_t currentTransitionIndex;
        bool isCompleted;
    };

    template <typename... Transitions>
        requires(transitions<Transitions...>)
    sequential_transition_chain sequential(Transitions... transitions);
} // namespace p5::animation

namespace p5::animation
{
    class parallel_transition_chain
    {
    public:
        template <typename... Transitions>
            requires(transitions<Transitions...>)
        explicit parallel_transition_chain(Transitions... transitions);

        advance_result advance(float deltaTimeInSeconds);
        transition_state get_transition_state() const;

    private:
        bool is_every_transition_completed() const;
        std::vector<std::unique_ptr<generic_transition>> transitions;
        std::vector<bool> transitionsCompleted;
    };

    template <typename... Transitions>
        requires(transitions<Transitions...>)
    constexpr parallel_transition_chain parallel(Transitions... transitions);
} // namespace p5::animation

namespace p5::animation
{
    class race_transition_chain
    {
    public:
        template <typename... Transitions>
            requires(transitions<Transitions...>)
        explicit race_transition_chain(Transitions... transitions);

        advance_result advance(float deltaTimeInSeconds);
        transition_state get_transition_state() const;

    private:
        std::vector<std::unique_ptr<generic_transition>> transitions;
        bool isCompleted;
    };

    template <typename... Transitions>
        requires(transitions<Transitions...>)
    race_transition_chain race(Transitions... transitions);
} // namespace p5::animation

namespace p5::animation
{
    class staggered_transition_chain
    {
    public:
        template <typename... Transitions>
            requires(transitions<Transitions...>)
        explicit staggered_transition_chain(float delay, Transitions... transitions);

        advance_result advance(float deltaTimeInSeconds);
        transition_state get_transition_state() const;

    private:
        bool is_every_transition_completed() const;

        std::vector<std::unique_ptr<generic_transition>> transitions;
        std::vector<float> pendingDelays;
        std::vector<bool> transitionsCompleted;
    };

    template <typename... Transitions>
        requires(transitions<Transitions...>)
    staggered_transition_chain staggered(float delay, Transitions... transitions);
} // namespace p5::animation

namespace p5::animation
{
    class animation_controller
    {
    public:
        template <transition Transition>
        void play(Transition transition)
        {
            transitions.push_back(std::make_unique<transition_wrapper<Transition>>(std::move(transition)));
        }

        void advance(float deltaTimeInSeconds)
        {
            std::erase_if(transitions, [deltaTimeInSeconds](std::unique_ptr<generic_transition>& transition) {
                return transition->advance(deltaTimeInSeconds).isCompleted;
            });
        }

    private:
        std::vector<std::unique_ptr<generic_transition>> transitions;
    };
} // namespace p5::animation

namespace p5::animation
{
    inline tween_transition::tween_transition(float duration, Curve curve, progress_transformer transformer)
        : durationInSeconds {std::max(duration, 0.0f)},
          elapsedTimeInSeconds {0.0f},
          curve {curve},
          transformer {std::move(transformer)},
          isCompleted {false}
    {
    }

    inline advance_result tween_transition::advance(float deltaTimeInSeconds)
    {
        const float previousElapsed = elapsedTimeInSeconds;
        elapsedTimeInSeconds = std::clamp(elapsedTimeInSeconds + deltaTimeInSeconds, 0.0f, durationInSeconds);
        const float timeConsumed = elapsedTimeInSeconds - previousElapsed;
        isCompleted = (deltaTimeInSeconds >= 0.0f) ? (elapsedTimeInSeconds >= durationInSeconds) : (elapsedTimeInSeconds <= 0.0f);

        const float progress = curve(durationInSeconds > 0.0f ? (elapsedTimeInSeconds / durationInSeconds) : 1.0f);
        transformer(progress);

        return {
            .timeConsumed = timeConsumed,
            .isCompleted = isCompleted
        };
    }

    inline transition_state tween_transition::get_transition_state() const
    {
        return isCompleted ? transition_state::completed : transition_state::animating;
    }
} // namespace p5::animation

namespace p5::animation
{
    inline tween_transition tween(float duration, Curve curve, progress_transformer transformer)
    {
        return tween_transition {duration, curve, std::move(transformer)};
    }
} // namespace p5::animation

namespace p5::animation
{
    inline wait_for_transition::wait_for_transition(float duration)
        : durationInSeconds {std::max(duration, 0.0f)},
          elapsedTimeInSeconds {0.0f},
          isCompleted {false}
    {
    }

    inline advance_result wait_for_transition::advance(float deltaTimeInSeconds)
    {
        const float previousElapsed = elapsedTimeInSeconds;
        elapsedTimeInSeconds = std::clamp(elapsedTimeInSeconds + deltaTimeInSeconds, 0.0f, durationInSeconds);
        const float timeConsumed = elapsedTimeInSeconds - previousElapsed;

        isCompleted = (deltaTimeInSeconds >= 0.0f) ? (elapsedTimeInSeconds >= durationInSeconds) : (elapsedTimeInSeconds <= 0.0f);

        return {
            .timeConsumed = timeConsumed,
            .isCompleted = isCompleted
        };
    }

    inline transition_state wait_for_transition::get_transition_state() const
    {
        return isCompleted ? transition_state::completed : transition_state::waiting;
    }
} // namespace p5::animation

namespace p5::animation
{
    inline wait_for_transition wait_for(float duration)
    {
        return wait_for_transition {duration};
    }
} // namespace p5::animation

namespace p5::animation
{
    inline wait_until_transition::wait_until_transition(condition_function condition)
        : condition {std::move(condition)},
          isCompleted {false}
    {
    }

    inline advance_result wait_until_transition::advance([[maybe_unused]] float deltaTimeInSeconds)
    {
        isCompleted = condition();

        return {
            .timeConsumed = 0.0f,
            .isCompleted = isCompleted
        };
    }

    inline transition_state wait_until_transition::get_transition_state() const
    {
        return isCompleted ? transition_state::completed : transition_state::waiting;
    }
} // namespace p5::animation

namespace p5::animation
{
    inline wait_until_transition wait_until(condition_function condition)
    {
        return wait_until_transition {std::move(condition)};
    }
} // namespace p5::animation

namespace p5::animation
{
    inline call_transition::call_transition(callback_function callback)
        : callback {std::move(callback)},
          isCompleted {false}
    {
    }

    inline advance_result call_transition::advance([[maybe_unused]] float deltaTimeInSeconds)
    {
        callback();
        isCompleted = true;

        return {
            .timeConsumed = 0.0f,
            .isCompleted = true
        };
    }

    inline transition_state call_transition::get_transition_state() const
    {
        return isCompleted ? transition_state::completed : transition_state::animating;
    }
} // namespace p5::animation

namespace p5::animation
{
    inline call_transition call(callback_function callback)
    {
        return call_transition {std::move(callback)};
    }
} // namespace p5::animation

namespace p5::animation
{
    template <typename Transition>
        requires transition<std::remove_cvref_t<Transition>>
    inline speed_adjusted_transition<Transition>::speed_adjusted_transition(Transition transition, float multiplier)
        : transition {std::move(transition)},
          multiplier {multiplier}
    {
    }

    template <typename Transition>
        requires transition<std::remove_cvref_t<Transition>>
    inline advance_result speed_adjusted_transition<Transition>::advance(float deltaTimeInSeconds)
    {
        return transition.advance(deltaTimeInSeconds * multiplier);
    }

    template <typename Transition>
        requires transition<std::remove_cvref_t<Transition>>
    inline transition_state speed_adjusted_transition<Transition>::get_transition_state() const
    {
        return transition.get_transition_state();
    }
} // namespace p5::animation

namespace p5::animation
{
    template <typename Transition>
        requires transition<std::remove_cvref_t<Transition>>
    inline speed_adjusted_transition<Transition> adjust_speed(Transition transition, float multiplier)
    {
        return speed_adjusted_transition<Transition> {std::move(transition), multiplier};
    }
} // namespace p5::animation

namespace p5::animation
{
    template <typename Factory>
        requires transition_factory<Factory>
    inline repeat_transition<Factory>::repeat_transition(Factory factory, std::optional<size_t> repeatCount)
        : factory {std::move(factory)},
          currentTransition {this->factory()},
          remainingCycles {repeatCount},
          isCompleted {false}
    {
    }

    template <typename Factory>
        requires transition_factory<Factory>
    inline advance_result repeat_transition<Factory>::advance(float deltaTimeInSeconds)
    {
        if (isCompleted) {
            return {
                .timeConsumed = 0.0f,
                .isCompleted = true
            };
        }

        const float initialDeltaTimeInSeconds = deltaTimeInSeconds;

        while (not isCompleted) {
            const advance_result result = currentTransition.advance(deltaTimeInSeconds);
            deltaTimeInSeconds -= result.timeConsumed;

            if (not result.isCompleted) {
                break;
            }

            if (remainingCycles.has_value()) {
                size_t& cycles = remainingCycles.value();
                if (cycles > 1) {
                    --cycles;
                } else {
                    isCompleted = true;
                }
            }

            currentTransition = factory();
        }

        return {
            .timeConsumed = std::max(initialDeltaTimeInSeconds - deltaTimeInSeconds, 0.0f),
            .isCompleted = isCompleted
        };
    }

    template <typename Factory>
        requires transition_factory<Factory>
    inline transition_state repeat_transition<Factory>::get_transition_state() const
    {
        return isCompleted ? transition_state::completed : currentTransition.get_transition_state();
    }
} // namespace p5::animation

namespace p5::animation
{
    template <typename Factory>
        requires transition_factory<Factory>
    inline repeat_transition<Factory> repeat(Factory factory, std::optional<size_t> repeatCount)
    {
        return repeat_transition<Factory> {std::move(factory), repeatCount};
    }
} // namespace p5::animation

namespace p5::animation
{
    template <typename Transition>
        requires transition<Transition>
    inline transition_wrapper<Transition>::transition_wrapper(Transition transition)
        : transition {std::move(transition)}
    {
    }

    template <typename Transition>
        requires transition<Transition>
    inline advance_result transition_wrapper<Transition>::advance(float deltaTimeInSeconds)
    {
        return transition.advance(deltaTimeInSeconds);
    }

    template <typename Transition>
        requires transition<Transition>
    inline transition_state transition_wrapper<Transition>::get_transition_state() const
    {
        return transition.get_transition_state();
    }
} // namespace p5::animation

namespace p5::animation
{
    template <typename... Transitions>
        requires(transitions<Transitions...>)
    inline sequential_transition_chain::sequential_transition_chain(Transitions... transitions)
        : transitions {},
          currentTransitionIndex {0},
          isCompleted {false}
    {
        this->transitions.reserve(sizeof...(Transitions));
        (this->transitions.push_back(std::make_unique<transition_wrapper<Transitions>>(std::forward<Transitions>(transitions))), ...);
    }

    inline advance_result sequential_transition_chain::advance(float deltaTimeInSeconds)
    {
        const float initialDeltaTime = deltaTimeInSeconds;

        while (not isCompleted) {
            const std::unique_ptr<generic_transition>& currentTransition = transitions[currentTransitionIndex];
            const advance_result result = currentTransition->advance(deltaTimeInSeconds);
            deltaTimeInSeconds -= result.timeConsumed;

            if (not result.isCompleted) {
                break; // Early out if the transition is not completed, as we don't want to advance to the next transition yet.
            }

            if (currentTransitionIndex + 1 < transitions.size()) {
                currentTransitionIndex++;
            } else {
                isCompleted = true;
            }
        }

        return {
            .timeConsumed = std::max(initialDeltaTime - deltaTimeInSeconds, 0.0f),
            .isCompleted = isCompleted
        };
    }

    inline transition_state sequential_transition_chain::get_transition_state() const
    {
        if (isCompleted) {
            return transition_state::completed;
        }

        return transitions[currentTransitionIndex]->get_transition_state();
    }
} // namespace p5::animation

namespace p5::animation
{
    template <typename... Transitions>
        requires(transitions<Transitions...>)
    inline sequential_transition_chain sequential(Transitions... transitions)
    {
        return sequential_transition_chain {std::forward<Transitions>(transitions)...};
    }
} // namespace p5::animation

namespace p5::animation
{
    template <typename... Transitions>
        requires(transitions<Transitions...>)
    inline parallel_transition_chain::parallel_transition_chain(Transitions... transitions)
        : transitions {},
          transitionsCompleted(sizeof...(Transitions), false)
    {
        this->transitions.reserve(sizeof...(Transitions));
        (this->transitions.push_back(std::make_unique<transition_wrapper<Transitions>>(std::forward<Transitions>(transitions))), ...);
    }

    inline advance_result parallel_transition_chain::advance(float deltaTimeInSeconds)
    {
        float timeConsumed = 0.0f;

        for (size_t i = 0; i < transitions.size(); ++i) {
            if (transitionsCompleted.at(i)) {
                continue;
            }

            const std::unique_ptr<generic_transition>& transition = transitions[i];
            const advance_result result = transition->advance(deltaTimeInSeconds);

            if (result.isCompleted) {
                transitionsCompleted[i] = true;
            }

            timeConsumed = std::max(timeConsumed, result.timeConsumed);
        }

        return {
            .timeConsumed = timeConsumed,
            .isCompleted = is_every_transition_completed()
        };
    }

    inline bool parallel_transition_chain::is_every_transition_completed() const
    {
        for (bool completed : transitionsCompleted) {
            if (not completed) {
                return false;
            }
        }

        return true;
    }

    inline transition_state parallel_transition_chain::get_transition_state() const
    {
        if (is_every_transition_completed()) {
            return transition_state::completed;
        }

        for (const std::unique_ptr<generic_transition>& transition : transitions) {
            if (transition->get_transition_state() == transition_state::animating) {
                return transition_state::animating;
            }
        }

        return transition_state::waiting;
    }
} // namespace p5::animation

namespace p5::animation
{
    template <typename... Transitions>
        requires(transitions<Transitions...>)
    inline constexpr parallel_transition_chain parallel(Transitions... transitions)
    {
        return parallel_transition_chain {std::forward<Transitions>(transitions)...};
    }
} // namespace p5::animation

namespace p5::animation
{
    template <typename... Transitions>
        requires(transitions<Transitions...>)
    inline race_transition_chain::race_transition_chain(Transitions... transitions)
        : transitions {},
          isCompleted {false}
    {
        this->transitions.reserve(sizeof...(Transitions));
        (this->transitions.push_back(std::make_unique<transition_wrapper<Transitions>>(std::forward<Transitions>(transitions))), ...);
    }

    inline advance_result race_transition_chain::advance(float deltaTimeInSeconds)
    {
        float timeConsumed = 0.0f;

        for (const std::unique_ptr<generic_transition>& transition : transitions) {
            if (isCompleted) {
                break;
            }

            const advance_result result = transition->advance(deltaTimeInSeconds);
            timeConsumed = std::max(timeConsumed, result.timeConsumed);

            if (result.isCompleted) {
                isCompleted = true;
            }
        }

        return {
            .timeConsumed = timeConsumed,
            .isCompleted = isCompleted
        };
    }

    inline transition_state race_transition_chain::get_transition_state() const
    {
        if (isCompleted) {
            return transition_state::completed;
        }

        for (const std::unique_ptr<generic_transition>& transition : transitions) {
            if (transition->get_transition_state() == transition_state::animating) {
                return transition_state::animating;
            }
        }

        return transition_state::waiting;
    }
} // namespace p5::animation

namespace p5::animation
{
    template <typename... Transitions>
        requires(transitions<Transitions...>)
    inline race_transition_chain race(Transitions... transitions)
    {
        return race_transition_chain {std::forward<Transitions>(transitions)...};
    }
} // namespace p5::animation

namespace p5::animation
{
    template <typename... Transitions>
        requires(transitions<Transitions...>)
    inline staggered_transition_chain::staggered_transition_chain(float delay, Transitions... transitions)
        : transitions {},
          pendingDelays {},
          transitionsCompleted(sizeof...(Transitions), false)
    {
        this->transitions.reserve(sizeof...(Transitions));
        pendingDelays.reserve(sizeof...(Transitions));

        (
            (this->transitions.push_back(std::make_unique<transition_wrapper<Transitions>>(std::forward<Transitions>(transitions))),
             pendingDelays.push_back(static_cast<float>(pendingDelays.size()) * std::max(delay, 0.0f))),
            ...);
    }

    inline advance_result staggered_transition_chain::advance(float deltaTimeInSeconds)
    {
        float timeConsumed = 0.0f;

        for (size_t i = 0; i < transitions.size(); ++i) {
            if (transitionsCompleted[i]) {
                continue;
            }

            float remainingDeltaTime = deltaTimeInSeconds;
            float localTimeConsumed = 0.0f;

            if (pendingDelays[i] > 0.0f) {
                const float delayConsumed = std::min(remainingDeltaTime, pendingDelays[i]);
                pendingDelays[i] -= delayConsumed;
                remainingDeltaTime -= delayConsumed;
                localTimeConsumed += delayConsumed;
            }

            if (pendingDelays[i] <= 0.0f and remainingDeltaTime > 0.0f) {
                const advance_result result = transitions[i]->advance(remainingDeltaTime);
                localTimeConsumed += result.timeConsumed;

                if (result.isCompleted) {
                    transitionsCompleted[i] = true;
                }
            }

            timeConsumed = std::max(timeConsumed, localTimeConsumed);
        }

        return {
            .timeConsumed = timeConsumed,
            .isCompleted = is_every_transition_completed()
        };
    }

    inline bool staggered_transition_chain::is_every_transition_completed() const
    {
        for (bool completed : transitionsCompleted) {
            if (not completed) {
                return false;
            }
        }

        return true;
    }

    inline transition_state staggered_transition_chain::get_transition_state() const
    {
        if (is_every_transition_completed()) {
            return transition_state::completed;
        }

        for (size_t i = 0; i < transitions.size(); ++i) {
            const bool started = pendingDelays[i] <= 0.0f;
            if (not transitionsCompleted[i] and started and transitions[i]->get_transition_state() == transition_state::animating) {
                return transition_state::animating;
            }
        }

        return transition_state::waiting;
    }
} // namespace p5::animation

namespace p5::animation
{
    template <typename... Transitions>
        requires(transitions<Transitions...>)
    inline staggered_transition_chain staggered(float delay, Transitions... transitions)
    {
        return staggered_transition_chain {delay, std::forward<Transitions>(transitions)...};
    }
} // namespace p5::animation
