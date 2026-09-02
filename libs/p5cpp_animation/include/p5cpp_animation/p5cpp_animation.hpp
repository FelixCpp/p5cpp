#pragma once

#include <variant>
#include <functional>
#include <algorithm>
#include <vector>
#include <concepts>
#include <optional>

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
    struct AdvanceResult
    {
        float timeConsumed; //< The amount of seconds consumed taken from the input time (deltaTimeInSeconds)
        bool isCompleted;   //< Whether the transition is completed
    };
} // namespace p5::animation

namespace p5::animation
{
    typedef std::function<void(float)> ProgressReceiver;

    class TweenTransition
    {
    public:
        constexpr explicit TweenTransition(float durationInSeconds, Curve curve, ProgressReceiver receiver);
        AdvanceResult advance(float deltaTimeInSeconds);

    private:
        float computeProgress() const;

        float m_durationInSeconds;
        float m_elapsedTimeInSeconds;
        Curve m_curve;
        ProgressReceiver m_receiver;

        bool m_isCompleted;
    };

    constexpr TweenTransition tween(float durationInSeconds, Curve curve, ProgressReceiver receiver);
} // namespace p5::animation

namespace p5::animation
{
    typedef std::function<void(float position)> SpringReceiver;

    class SpringTransition
    {
    public:
        constexpr explicit SpringTransition(
            float stiffness,
            float damping,
            float mass,
            float initialPosition,
            float targetPosition,
            float initialVelocity,
            SpringReceiver receiver
        );

        AdvanceResult advance(float deltaTimeInSeconds);

    private:
        float m_stiffness;
        float m_damping;
        float m_mass;
        float m_velocity;
        float m_position;
        float m_targetPosition;
        float m_elapsedTimeInSeconds;

        SpringReceiver m_receiver;

        bool m_isCompleted;
    };

    // Common case: critically-damped, unit-mass spring starting at rest.
    constexpr SpringTransition spring(
        float stiffness,
        float damping,
        float initialPosition,
        float targetPosition,
        SpringReceiver receiver
    );

    // Advanced case: full control over mass and initial velocity (e.g. for
    // "flinging" a value that already carries momentum from a gesture).
    constexpr SpringTransition spring(
        float stiffness,
        float damping,
        float mass,
        float initialVelocity,
        float initialPosition,
        float targetPosition,
        SpringReceiver receiver
    );
} // namespace p5::animation

namespace p5::animation
{
    class WaitForTransition
    {
    public:
        constexpr explicit WaitForTransition(float durationInSeconds);
        AdvanceResult advance(float deltaTimeInSeconds);

    private:
        float m_durationInSeconds;
        float m_elapsedTimeInSeconds;

        bool m_isDoneWaiting;
    };

    constexpr WaitForTransition waitFor(float durationInSeconds);
} // namespace p5::animation

namespace p5::animation
{
    typedef std::function<bool(float elapsedTimeInSeconds)> WaitUntilCondition;

    class WaitUntilTransition
    {
    public:
        constexpr explicit WaitUntilTransition(WaitUntilCondition condition);
        AdvanceResult advance(float deltaTimeInSeconds);

    private:
        WaitUntilCondition m_condition;
        float m_elapsedTimeInSeconds;

        bool m_isDoneWaiting;
    };

    constexpr WaitUntilTransition waitUntil(WaitUntilCondition condition);
} // namespace p5::animation

namespace p5::animation
{
    typedef std::function<void()> VoidCallback;

    class InterceptTransition
    {
    public:
        constexpr explicit InterceptTransition(VoidCallback callback);
        AdvanceResult advance(float deltaTime);

    private:
        VoidCallback m_callback;
        bool m_isCompleted;
    };

    constexpr InterceptTransition intercept(VoidCallback callback);
} // namespace p5::animation

namespace p5::animation
{
    class TransitionEvent;

    typedef std::vector<TransitionEvent> TransitionEvents;
} // namespace p5::animation

namespace p5::animation
{
    class SequentialTransitionComposite
    {
    public:
        explicit constexpr SequentialTransitionComposite(TransitionEvents events);
        AdvanceResult advance(float deltaTimeInSeconds);

    private:
        TransitionEvents m_events;
        size_t m_currentTransitionIndex;

        bool m_isCompleted;
    };

    constexpr SequentialTransitionComposite sequential(TransitionEvents events);
} // namespace p5::animation

namespace p5::animation
{
    class ParallelTransitionComposite
    {
    public:
        explicit constexpr ParallelTransitionComposite(TransitionEvents events);
        AdvanceResult advance(float deltaTimeInSeconds);

    private:
        TransitionEvents m_events;
        std::vector<bool> m_completedEvents;
        bool m_isCompleted;
    };

    constexpr ParallelTransitionComposite parallel(TransitionEvents events);
} // namespace p5::animation

namespace p5::animation
{
    class RaceTransitionComposite
    {
    public:
        explicit constexpr RaceTransitionComposite(TransitionEvents events);
        AdvanceResult advance(float deltaTimeInSeconds);

    private:
        TransitionEvents m_events;
        bool m_isCompleted;
    };

    constexpr RaceTransitionComposite race(TransitionEvents events);
} // namespace p5::animation

namespace p5::animation
{
    class RepeatingTransitionComposite;
} // namespace p5::animation

namespace p5::animation
{
    // clang-format off
    using TransitionType = std::variant<
        TweenTransition,
        SpringTransition,
        WaitForTransition,
        WaitUntilTransition,
        InterceptTransition,
        SequentialTransitionComposite,
        ParallelTransitionComposite,
        RaceTransitionComposite,
        RepeatingTransitionComposite
    >;
    // clang-format on
} // namespace p5::animation

namespace p5::animation
{
    using EventFactory = std::function<TransitionEvent()>;

    class RepeatingTransitionComposite
    {
    public:
        constexpr explicit RepeatingTransitionComposite(EventFactory factory, std::optional<size_t> repeatCount);
        AdvanceResult advance(float deltaTimeInSeconds);

    private:
        EventFactory m_factory;
        TransitionEvents m_current;
        std::optional<size_t> m_repeatCount;
        size_t m_completedRepeats;
        bool m_isCompleted;
    };

    template <typename T>
        requires std::constructible_from<TransitionType, T>
    constexpr RepeatingTransitionComposite repeating(T&& transition, std::optional<size_t> repeatCount = std::nullopt);
} // namespace p5::animation

namespace p5::animation
{
    class TransitionEvent
    {
    public:
        template <typename T>
            requires std::constructible_from<TransitionType, T>
        constexpr TransitionEvent(T&& transition);
        AdvanceResult advance(float deltaTimeInSeconds);

    private:
        TransitionType m_type;
    };
} // namespace p5::animation

namespace p5::animation
{
    inline constexpr TweenTransition::TweenTransition(float durationInSeconds, Curve curve, ProgressReceiver receiver)
        : m_durationInSeconds {durationInSeconds},
          m_elapsedTimeInSeconds {0.0f},
          m_curve {curve},
          m_receiver {std::move(receiver)},
          m_isCompleted {false}
    {
    }

    inline AdvanceResult TweenTransition::advance(const float deltaTimeInSeconds)
    {
        if (m_isCompleted) {
            return {
                .timeConsumed = 0.0f,
                .isCompleted = true,
            };
        }

        const float remainingTime = m_durationInSeconds - m_elapsedTimeInSeconds;
        const float advanceTime = std::min(remainingTime, deltaTimeInSeconds);
        const bool isCompletedAfterAdvancing = (m_elapsedTimeInSeconds + deltaTimeInSeconds) >= m_durationInSeconds;

        m_elapsedTimeInSeconds = std::clamp(m_elapsedTimeInSeconds + advanceTime, 0.0f, m_durationInSeconds); // We dont want the elapsed time to go out of bounds

        const float rawProgress = computeProgress();
        const float easedProgress = m_curve(rawProgress);
        m_receiver(easedProgress);

        m_isCompleted = isCompletedAfterAdvancing;

        return {
            .timeConsumed = advanceTime,
            .isCompleted = isCompletedAfterAdvancing,
        };
    }

    inline float TweenTransition::computeProgress() const
    {
        if (m_durationInSeconds <= 0.0f) {
            return 1.0f;
        }

        return m_elapsedTimeInSeconds / m_durationInSeconds;
    }
} // namespace p5::animation

namespace p5::animation
{
    inline constexpr TweenTransition tween(float durationInSeconds, Curve curve, ProgressReceiver receiver)
    {
        return TweenTransition {
            durationInSeconds,
            curve,
            std::move(receiver)
        };
    }
} // namespace p5::animation

namespace p5::animation
{
    inline constexpr SpringTransition::SpringTransition(float stiffness, float damping, float mass, float initialPosition, float targetPosition, float initialVelocity, SpringReceiver receiver)
        : m_stiffness {stiffness},
          m_damping {damping},
          m_mass {mass},
          m_velocity {initialVelocity},
          m_position {initialPosition},
          m_targetPosition {targetPosition},
          m_elapsedTimeInSeconds {0.0f},
          m_receiver {std::move(receiver)},
          m_isCompleted {false}
    {
    }

    inline AdvanceResult SpringTransition::advance(const float deltaTimeInSeconds)
    {
        if (m_isCompleted) {
            return {
                .timeConsumed = 0.0f,
                .isCompleted = true,
            };
        }

        // Calculate the spring force
        const float displacement = m_position - m_targetPosition;
        const float springForce = -m_stiffness * displacement;

        // Calculate the damping force
        const float dampingForce = -m_damping * m_velocity;

        // Calculate the acceleration
        const float acceleration = (springForce + dampingForce) / m_mass;

        // Update velocity and position using simple Euler integration
        m_velocity += acceleration * deltaTimeInSeconds;
        m_position += m_velocity * deltaTimeInSeconds;
        m_receiver(m_position);

        m_elapsedTimeInSeconds += deltaTimeInSeconds;

        // Check if the spring has settled (velocity is small and close to target)
        m_isCompleted = std::abs(m_velocity) < 0.001f && std::abs(displacement) < 0.001f;

        return {
            .timeConsumed = deltaTimeInSeconds,
            .isCompleted = m_isCompleted,
        };
    }
} // namespace p5::animation

namespace p5::animation
{
    inline constexpr SpringTransition spring(float stiffness, float damping, float initialPosition, float targetPosition, SpringReceiver receiver)
    {
        return SpringTransition {stiffness, damping, 1.0f, initialPosition, targetPosition, 0.0f, std::move(receiver)};
    }

    inline constexpr SpringTransition spring(float stiffness, float damping, float mass, float initialVelocity, float initialPosition, float targetPosition, SpringReceiver receiver)
    {
        return SpringTransition {stiffness, damping, mass, initialPosition, targetPosition, initialVelocity, std::move(receiver)};
    }
} // namespace p5::animation

namespace p5::animation
{
    inline constexpr WaitForTransition::WaitForTransition(const float durationInSeconds)
        : m_durationInSeconds {durationInSeconds},
          m_elapsedTimeInSeconds {0.0f},
          m_isDoneWaiting {false}
    {
    }

    inline AdvanceResult WaitForTransition::advance(const float deltaTimeInSeconds)
    {
        if (m_isDoneWaiting) {
            return {
                .timeConsumed = 0.0f,
                .isCompleted = true,
            };
        }

        const float remainingTime = m_durationInSeconds - m_elapsedTimeInSeconds;
        const float advanceTime = std::min(remainingTime, deltaTimeInSeconds);
        const bool isDoneWaitingAfterAdvancing = (m_elapsedTimeInSeconds + deltaTimeInSeconds) >= m_durationInSeconds;

        m_elapsedTimeInSeconds = std::clamp(m_elapsedTimeInSeconds + advanceTime, 0.0f, m_durationInSeconds);
        m_isDoneWaiting = isDoneWaitingAfterAdvancing;

        return {
            .timeConsumed = advanceTime,
            .isCompleted = m_isDoneWaiting,
        };
    }
} // namespace p5::animation

namespace p5::animation
{
    inline constexpr WaitForTransition waitFor(const float durationInSeconds)
    {
        return WaitForTransition {durationInSeconds};
    }
} // namespace p5::animation

namespace p5::animation
{
    inline constexpr WaitUntilTransition::WaitUntilTransition(WaitUntilCondition condition)
        : m_condition {std::move(condition)},
          m_elapsedTimeInSeconds {0.0f},
          m_isDoneWaiting {false}
    {
    }

    inline AdvanceResult WaitUntilTransition::advance(const float deltaTimeInSeconds)
    {
        if (m_isDoneWaiting) {
            return {
                .timeConsumed = 0.0f,
                .isCompleted = true,
            };
        }

        m_elapsedTimeInSeconds += deltaTimeInSeconds;
        m_isDoneWaiting = m_condition(m_elapsedTimeInSeconds);

        return {
            .timeConsumed = 0.0f,
            .isCompleted = m_isDoneWaiting,
        };
    }
} // namespace p5::animation

namespace p5::animation
{
    inline constexpr WaitUntilTransition waitUntil(WaitUntilCondition condition)
    {
        return WaitUntilTransition {std::move(condition)};
    }
} // namespace p5::animation

namespace p5::animation
{
    inline constexpr InterceptTransition::InterceptTransition(VoidCallback callback)
        : m_callback {std::move(callback)},
          m_isCompleted {false}
    {
    }

    inline AdvanceResult InterceptTransition::advance([[maybe_unused]] const float deltaTime)
    {
        if (not m_isCompleted) {
            m_callback();
        }

        return {
            .timeConsumed = 0.0f,
            .isCompleted = true,
        };
    }
} // namespace p5::animation

namespace p5::animation
{
    inline constexpr InterceptTransition intercept(VoidCallback callback)
    {
        return InterceptTransition {std::move(callback)};
    }
} // namespace p5::animation

namespace p5::animation
{
    template <typename... T>
    struct visitors : T...
    {
        using T::operator()...;
    };

    template <typename T>
        requires std::constructible_from<TransitionType, T>
    inline constexpr TransitionEvent::TransitionEvent(T&& transition)
        : m_type {std::forward<T>(transition)}
    {
    }

    inline AdvanceResult TransitionEvent::advance(const float deltaTimeInSeconds)
    {
        return std::visit(
            visitors {
                [deltaTimeInSeconds](TweenTransition& transition) {
                    return transition.advance(deltaTimeInSeconds);
                },
                [deltaTimeInSeconds](SpringTransition& transition) {
                    return transition.advance(deltaTimeInSeconds);
                },
                [deltaTimeInSeconds](WaitForTransition& transition) {
                    return transition.advance(deltaTimeInSeconds);
                },
                [deltaTimeInSeconds](WaitUntilTransition& transition) {
                    return transition.advance(deltaTimeInSeconds);
                },
                [deltaTimeInSeconds](InterceptTransition& transition) {
                    return transition.advance(deltaTimeInSeconds);
                },
                [deltaTimeInSeconds](SequentialTransitionComposite& transition) {
                    return transition.advance(deltaTimeInSeconds);
                },
                [deltaTimeInSeconds](ParallelTransitionComposite& transition) {
                    return transition.advance(deltaTimeInSeconds);
                },
                [deltaTimeInSeconds](RaceTransitionComposite& transition) {
                    return transition.advance(deltaTimeInSeconds);
                },
                [deltaTimeInSeconds](RepeatingTransitionComposite& transition) {
                    return transition.advance(deltaTimeInSeconds);
                },
            },
            m_type
        );
    }
} // namespace p5::animation

namespace p5::animation
{
    inline constexpr SequentialTransitionComposite::SequentialTransitionComposite(TransitionEvents events)
        : m_events {std::move(events)},
          m_currentTransitionIndex {0uz},
          m_isCompleted {false}
    {
    }

    inline AdvanceResult SequentialTransitionComposite::advance(const float deltaTimeInSeconds)
    {
        if (m_isCompleted) {
            return {
                .timeConsumed = 0.0f,
                .isCompleted = true,
            };
        }

        float timeLeftThisFrame = deltaTimeInSeconds;

        while (not m_isCompleted) {
            const AdvanceResult result = m_events[m_currentTransitionIndex].advance(timeLeftThisFrame);
            timeLeftThisFrame = std::max(timeLeftThisFrame - result.timeConsumed, 0.0f);

            if (not result.isCompleted) {
                break;
            }

            const bool hasNextTransition = (m_currentTransitionIndex + 1) < m_events.size();
            if (hasNextTransition) {
                ++m_currentTransitionIndex;
            } else {
                m_isCompleted = true;
            }
        }

        return {
            .timeConsumed = deltaTimeInSeconds - timeLeftThisFrame,
            .isCompleted = m_isCompleted,
        };
    }
} // namespace p5::animation

namespace p5::animation
{
    inline constexpr SequentialTransitionComposite sequential(TransitionEvents events)
    {
        return SequentialTransitionComposite {
            std::move(events)
        };
    }
} // namespace p5::animation

namespace p5::animation
{
    inline constexpr ParallelTransitionComposite::ParallelTransitionComposite(TransitionEvents events)
        : m_events {std::move(events)},
          m_completedEvents(m_events.size(), false),
          m_isCompleted {false}
    {
    }

    inline AdvanceResult ParallelTransitionComposite::advance(const float deltaTimeInSeconds)
    {
        if (m_isCompleted) {
            return {
                .timeConsumed = 0.0f,
                .isCompleted = true,
            };
        }

        float timeConsumed = 0.0f;
        bool allCompleted = true;

        for (size_t i = 0; i < m_events.size(); ++i) {
            if (m_completedEvents[i]) {
                continue;
            }

            const AdvanceResult result = m_events[i].advance(deltaTimeInSeconds);
            timeConsumed = std::max(timeConsumed, result.timeConsumed);

            if (result.isCompleted) {
                m_completedEvents[i] = true;
            } else {
                allCompleted = false;
            }
        }

        m_isCompleted = allCompleted;

        return {
            .timeConsumed = timeConsumed,
            .isCompleted = m_isCompleted,
        };
    }
} // namespace p5::animation

namespace p5::animation
{
    inline constexpr ParallelTransitionComposite parallel(TransitionEvents events)
    {
        return ParallelTransitionComposite {
            std::move(events)
        };
    }
} // namespace p5::animation

namespace p5::animation
{
    inline constexpr RaceTransitionComposite::RaceTransitionComposite(TransitionEvents events)
        : m_events {std::move(events)},
          m_isCompleted {false}
    {
    }

    inline AdvanceResult RaceTransitionComposite::advance(const float deltaTimeInSeconds)
    {
        if (m_isCompleted) {
            return {
                .timeConsumed = 0.0f,
                .isCompleted = true,
            };
        }

        float timeConsumed = 0.0f;
        bool anyCompleted = false;

        for (auto& event : m_events) {
            const AdvanceResult result = event.advance(deltaTimeInSeconds);
            timeConsumed = std::max(timeConsumed, result.timeConsumed);

            if (result.isCompleted) {
                anyCompleted = true;
            }
        }

        m_isCompleted = anyCompleted;

        return {
            .timeConsumed = timeConsumed,
            .isCompleted = m_isCompleted,
        };
    }
} // namespace p5::animation

namespace p5::animation
{
    inline constexpr RaceTransitionComposite race(TransitionEvents events)
    {
        return RaceTransitionComposite {
            std::move(events)
        };
    }
} // namespace p5::animation

namespace p5::animation
{
    inline constexpr RepeatingTransitionComposite::RepeatingTransitionComposite(
        EventFactory factory,
        std::optional<size_t> repeatCount
    ) : m_factory {std::move(factory)},
        m_current {m_factory()},
        m_repeatCount {repeatCount},
        m_completedRepeats {0uz},
        m_isCompleted {false}
    {
    }

    inline AdvanceResult RepeatingTransitionComposite::advance(const float deltaTimeInSeconds)
    {
        if (m_isCompleted) {
            return {
                .timeConsumed = 0.0f,
                .isCompleted = true,
            };
        }

        // Safety net: a transition that is repeated indefinitely and always
        // completes without consuming any time (e.g. `intercept`/`waitUntil`)
        // would otherwise spin forever inside a single call. Capping the
        // number of cycles processed per `advance()` call turns that
        // degenerate case into "keep repeating across frames" instead of
        // freezing the draw loop.
        static constexpr size_t maxCyclesPerAdvance = 10'000uz;

        float timeLeftThisFrame = deltaTimeInSeconds;

        for (size_t cycle = 0uz; cycle < maxCyclesPerAdvance; ++cycle) {
            const AdvanceResult result = m_current.front().advance(timeLeftThisFrame);
            timeLeftThisFrame = std::max(timeLeftThisFrame - result.timeConsumed, 0.0f);

            if (not result.isCompleted) {
                break;
            }

            ++m_completedRepeats;
            if (m_repeatCount.has_value() && m_completedRepeats >= *m_repeatCount) {
                m_isCompleted = true;
                break;
            }

            m_current.front() = m_factory();
        }

        return {
            .timeConsumed = deltaTimeInSeconds - timeLeftThisFrame,
            .isCompleted = m_isCompleted,
        };
    }
} // namespace p5::animation

namespace p5::animation
{
    template <typename T>
        requires std::constructible_from<TransitionType, T>
    inline constexpr RepeatingTransitionComposite repeating(T&& transition, std::optional<size_t> repeatCount)
    {
        return RepeatingTransitionComposite {
            [transition = std::forward<T>(transition)]() {
                return TransitionEvent {transition};
            },
            repeatCount
        };
    }
} // namespace p5::animation
