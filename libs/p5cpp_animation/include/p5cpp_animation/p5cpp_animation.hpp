#pragma once

#include <variant>
#include <functional>
#include <algorithm>
#include <vector>
#include <concepts>

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
        explicit constexpr TweenTransition(float durationInSeconds, Curve curve, ProgressReceiver receiver)
            : m_durationInSeconds {durationInSeconds},
              m_elapsedTimeInSeconds {0.0f},
              m_curve {curve},
              m_receiver {std::move(receiver)},
              m_isCompleted {false}
        {
        }

        AdvanceResult advance(const float deltaTimeInSeconds)
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

    private:
        float computeProgress() const
        {
            if (m_durationInSeconds <= 0.0f) {
                return 1.0f;
            }

            return m_elapsedTimeInSeconds / m_durationInSeconds;
        }

        float m_durationInSeconds;
        float m_elapsedTimeInSeconds;
        Curve m_curve;
        ProgressReceiver m_receiver;

        bool m_isCompleted;
    };

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
    class WaitForTransition
    {
    public:
        constexpr explicit WaitForTransition(const float durationInSeconds)
            : m_durationInSeconds {durationInSeconds},
              m_elapsedTimeInSeconds {0.0f},
              m_isDoneWaiting {false}
        {
        }

        AdvanceResult advance(const float deltaTimeInSeconds)
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

    private:
        float m_durationInSeconds;
        float m_elapsedTimeInSeconds;

        bool m_isDoneWaiting;
    };

    inline constexpr WaitForTransition waitFor(const float durationInSeconds)
    {
        return WaitForTransition {durationInSeconds};
    }
} // namespace p5::animation

namespace p5::animation
{
    typedef std::function<bool(float elapsedTimeInSeconds)> WaitUntilCondition;

    class WaitUntilTransition
    {
    public:
        constexpr explicit WaitUntilTransition(WaitUntilCondition condition)
            : m_condition {std::move(condition)},
              m_elapsedTimeInSeconds {0.0f},
              m_isDoneWaiting {false}
        {
        }

        AdvanceResult advance(const float deltaTimeInSeconds)
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

    private:
        WaitUntilCondition m_condition;
        float m_elapsedTimeInSeconds;

        bool m_isDoneWaiting;
    };

    inline constexpr WaitUntilTransition waitUntil(WaitUntilCondition condition)
    {
        return WaitUntilTransition {std::move(condition)};
    }
} // namespace p5::animation

namespace p5::animation
{
    typedef std::function<void()> VoidCallback;

    class InterceptTransition
    {
    public:
        constexpr explicit InterceptTransition(VoidCallback callback)
            : m_callback {std::move(callback)},
              m_isCompleted {false}
        {
        }

        AdvanceResult advance(const float deltaTime)
        {
            if (not m_isCompleted) {
                m_callback();
            }

            return {
                .timeConsumed = 0.0f,
                .isCompleted = true,
            };
        }

    private:
        VoidCallback m_callback;
        bool m_isCompleted;
    };

    inline constexpr InterceptTransition intercept(VoidCallback callback)
    {
        return InterceptTransition {std::move(callback)};
    }
} // namespace p5::animation

namespace p5::animation
{
    class TransitionEvent
    {
    public:
        using TransitionType = std::variant<TweenTransition, WaitForTransition, WaitUntilTransition, InterceptTransition>;

        template <typename T>
            requires std::constructible_from<TransitionType, T>
        constexpr TransitionEvent(T&& transition)
            : m_type {std::forward<T>(transition)}
        {
        }

        AdvanceResult advance(const float deltaTimeInSeconds)
        {
            return std::visit(
                [deltaTimeInSeconds](auto& transition) {
                    return transition.advance(deltaTimeInSeconds);
                },
                m_type
            );
        }

    private:
        TransitionType m_type;
    };

    typedef std::vector<TransitionEvent> TransitionEvents;

    class SequentialTransitionComposite
    {
    public:
        explicit constexpr SequentialTransitionComposite(TransitionEvents events)
            : m_events {std::move(events)},
              m_currentTransitionIndex {0uz},
              m_completedEvents(m_events.size(), false)
        {
        }

        AdvanceResult advance(const float deltaTimeInSeconds)
        {
            if (m_isCompleted) {
                return AdvanceResult {
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

    private:
        TransitionEvents m_events;
        size_t m_currentTransitionIndex;
        std::vector<bool> m_completedEvents;

        bool m_isCompleted;
    };

    inline constexpr SequentialTransitionComposite sequential(TransitionEvents events)
    {
        return SequentialTransitionComposite {
            std::move(events)
        };
    }
} // namespace p5::animation
