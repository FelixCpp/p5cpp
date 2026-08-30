#pragma once

#include <p5cpp/p5cpp.hpp>

#include <algorithm>
#include <cassert>
#include <functional>

namespace p5::animation
{
    enum class LoopMode
    {
        once,
        loop,
        pingpong,
    };

    enum class PlayState
    {
        initial,
        playing,
        paused,
        finished,
    };
} // namespace p5::animation

namespace p5::animation
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

    typedef float (*EasingFunction)(float);
} // namespace p5::animation

namespace p5::animation
{
    template <typename T> struct tween
    {
        T from;                // Starting value of the tween
        T to;                  // Destination value of the tween
        float duration;        // Duration of the tween in seconds
        EasingFunction easing; // Easing function to apply to the tween
        LoopMode loopMode;     // Looping behavior of the tween
        int repeatCount;       // Number of end-to-end traversals to play while looping, then settle in the finished state; -1 = infinite (default)

        float elapsedTime;    // Elapsed time since the start of the tween in seconds
        bool isReversing;     // Flag indicating whether the tween is currently reversing (for pingpong mode)
        PlayState state;      // Current state of the tween (playing, paused, or stopped)
        int repeatsCompleted; // Traversals completed so far; reset by restart()/reset()
    };

    template <typename T> tween<T> createTween(const T& from, const T& to, float duration, EasingFunction easing = &easeInOutSine, LoopMode loopMode = LoopMode::once);
    template <typename T> void restart(tween<T>& tween);
    template <typename T> void reset(tween<T>& tween);
    template <typename T> void pause(tween<T>& tween);
    template <typename T> void resume(tween<T>& tween);
    template <typename T> void advance(tween<T>& tween, float deltaTime);
    template <typename T> void loop(tween<T>& tween, LoopMode loopMode, int repeatCount = -1);

    // Redirects a tween towards a new target: `from` becomes its current value, so playback continues
    // smoothly from wherever it is right now instead of jumping. Unlike spring, this does not carry over
    // velocity - the eased approach to the new target simply restarts from elapsedTime = 0.
    template <typename T> void retarget(tween<T>& tween, const T& to);

    // Checks isFinished() and, if true, immediately resets the tween to its initial state before
    // returning true. Prefer this over isFinished() when you act on completion (e.g. to swap to the
    // next animation), so a finished tween can never be read again with its stale end value.
    template <typename T> bool consumeFinished(tween<T>& tween);

    template <typename T> T value(const tween<T>& tween);
    template <typename T> float progress(const tween<T>& tween);
    template <typename T> bool isInitial(const tween<T>& tween);
    template <typename T> bool isPlaying(const tween<T>& tween);
    template <typename T> bool isPaused(const tween<T>& tween);
    template <typename T> bool isFinished(const tween<T>& tween);
} // namespace p5::animation

namespace p5::animation
{
    template <typename T> struct timeline_entry
    {
        T to;
        float duration;
        EasingFunction easing;
    };

    template <typename T> struct timeline
    {
        T from;
        float totalDuration;
        std::vector<timeline_entry<T>> entries;

        float elapsedTime;
        LoopMode loopMode;
        int repeatCount; // Number of end-to-end traversals to play while looping, then settle in the finished state; -1 = infinite (default)
        bool isReversing;
        PlayState state;
        int repeatsCompleted; // Traversals completed so far; reset by restart()/reset()
    };

    template <typename T> timeline<T> createTimeline(const T& from, const std::vector<timeline_entry<T>>& entries);
    template <typename T> void restart(timeline<T>& timeline);
    template <typename T> void reset(timeline<T>& timeline);
    template <typename T> void pause(timeline<T>& timeline);
    template <typename T> void resume(timeline<T>& timeline);
    template <typename T> void advance(timeline<T>& timeline, float deltaTime);
    template <typename T> void loop(timeline<T>& timeline, LoopMode loopMode, int repeatCount = -1);

    // See tween's consumeFinished() - same contract, applied to a timeline.
    template <typename T> bool consumeFinished(timeline<T>& timeline);

    template <typename T> T value(const timeline<T>& timeline);
    template <typename T> float progress(const timeline<T>& timeline);
    template <typename T> bool isInitial(const timeline<T>& timeline);
    template <typename T> bool isPlaying(const timeline<T>& timeline);
    template <typename T> bool isPaused(const timeline<T>& timeline);
    template <typename T> bool isFinished(const timeline<T>& timeline);
} // namespace p5::animation

namespace p5::animation
{
    template <typename T> struct spring
    {
        T from;            // Starting value of the spring
        T to;              // Target value of the spring
        float stiffness;   // Spring constant: higher values react faster to the target
        float damping;     // Damping coefficient: higher values reduce oscillation/overshoot
        float mass;        // Simulated mass: higher values react slower to the target
        LoopMode loopMode; // Looping behavior of the spring, evaluated once it has settled
        int repeatCount;   // Number of times to settle-and-repeat before finishing; -1 = infinite (default)

        T velocity;           // Current velocity of the spring simulation
        T position;           // Current position of the spring simulation (the animated value)
        PlayState state;      // Current state of the spring (playing, paused, or stopped)
        bool isReversing;     // Flag indicating whether the spring is currently reversing (for pingpong mode)
        int repeatsCompleted; // Settles completed so far; reset by restart()/reset()
    };

    template <typename T> spring<T> createSpring(const T& from, const T& to, float stiffness = 170.0f, float damping = 26.0f, float mass = 1.0f, LoopMode loopMode = LoopMode::once);
    template <typename T> void restart(spring<T>& spring);
    template <typename T> void reset(spring<T>& spring);
    template <typename T> void pause(spring<T>& spring);
    template <typename T> void resume(spring<T>& spring);
    template <typename T> void advance(spring<T>& spring, float deltaTime);
    template <typename T> void loop(spring<T>& spring, LoopMode loopMode, int repeatCount = -1);

    // See tween's consumeFinished() - same contract, applied to a spring.
    template <typename T> bool consumeFinished(spring<T>& spring);

    template <typename T> T value(const spring<T>& spring);
    template <typename T> float progress(const spring<T>& spring);
    template <typename T> bool isInitial(const spring<T>& spring);
    template <typename T> bool isPlaying(const spring<T>& spring);
    template <typename T> bool isPaused(const spring<T>& spring);
    template <typename T> bool isFinished(const spring<T>& spring);
} // namespace p5::animation

namespace p5::animation
{
    // One link in a sequence: either a fixed `duration` (wait()), or a dynamic `isDone` predicate
    // that overrides it (play()). `onEnter` fires once when the step becomes active, `onAdvance`
    // fires every frame while it is active. All three are optional (may be empty/nullptr).
    struct sequence_step
    {
        float duration = 0.0f;
        std::function<void()> onEnter = nullptr;
        std::function<void(float)> onAdvance = nullptr;
        std::function<bool()> isDone = nullptr;
    };

    // A cursor that plays a list of heterogeneous steps one after another. Unlike tween/timeline/
    // spring, it does not interpolate any value itself - it only decides *when* each step is done
    // and hands control to the next one.
    //
    // LoopMode::once finishes after a single pass through *this sequence's own step list* - not
    // after "one full rotation" of whatever content a call()/play() step happens to cycle through
    // (sequence has no notion of that). To stop after N repeats of the whole step list (e.g. once
    // per item in a list you are rotating through), use loop(seq, LoopMode::loop, N) instead.
    //
    // LoopMode::pingpong has no well-defined meaning for a chain of one-shot side effects (you
    // cannot "run a callback backwards"), so loop() rejects it via assert rather than silently
    // reinterpreting it as something else.
    struct sequence
    {
        std::vector<sequence_step> steps;

        size_t stepIndex;
        float stepElapsedTime;
        bool stepEntered; // whether onEnter has already fired for the current step

        LoopMode loopMode;
        int repeatCount; // Number of end-to-end passes through all steps before finishing; -1 = infinite (default)
        PlayState state;
        int repeatsCompleted;
    };

    sequence createSequence(const std::vector<sequence_step>& steps);
    void restart(sequence& sequence);
    void reset(sequence& sequence);
    void pause(sequence& sequence);
    void resume(sequence& sequence);
    void advance(sequence& sequence, float deltaTime);

    // Asserts loopMode != LoopMode::pingpong - see the comment on sequence::loopMode for why.
    void loop(sequence& sequence, LoopMode loopMode, int repeatCount = -1);

    // See tween's consumeFinished() - same contract, applied to a sequence.
    bool consumeFinished(sequence& sequence);

    // Index of the currently active step. Meant for introspection/debugging - reading the actual
    // animated value of a play()ed tween/spring/timeline should happen directly on that object.
    size_t currentStep(const sequence& sequence);

    bool isInitial(const sequence& sequence);
    bool isPlaying(const sequence& sequence);
    bool isPaused(const sequence& sequence);
    bool isFinished(const sequence& sequence);

    // --- DSL for building sequence_steps ---

    // Waits `seconds` without doing anything.
    sequence_step wait(float seconds);

    // Runs `action` once, instantly, then moves on to the next step in the same advance() call.
    sequence_step call(std::function<void()> action);

    // Restarts `tween`/`spring`/`timeline` on entering this step, advances it every frame, and
    // moves on once it reports isFinished(). The wrapped object is captured by reference and must
    // outlive the sequence.
    template <typename T> sequence_step play(tween<T>& tween);
    template <typename T> sequence_step play(spring<T>& spring);
    template <typename T> sequence_step play(timeline<T>& timeline);
} // namespace p5::animation

namespace p5::animation::detail
{
    // Advances a single scalar spring simulation step via semi-implicit (symplectic) Euler
    // integration. Overloaded per vector type below, component-wise.
    inline void springStep(float& position, float& velocity, float target, float stiffness, float damping, float mass, float deltaTime)
    {
        const float acceleration = (stiffness * (target - position) - damping * velocity) / mass;
        velocity += acceleration * deltaTime;
        position += velocity * deltaTime;
    }

    inline void springStep(float2& position, float2& velocity, const float2& target, float stiffness, float damping, float mass, float deltaTime)
    {
        springStep(position.x, velocity.x, target.x, stiffness, damping, mass, deltaTime);
        springStep(position.y, velocity.y, target.y, stiffness, damping, mass, deltaTime);
    }

    inline void springStep(float3& position, float3& velocity, const float3& target, float stiffness, float damping, float mass, float deltaTime)
    {
        springStep(position.x, velocity.x, target.x, stiffness, damping, mass, deltaTime);
        springStep(position.y, velocity.y, target.y, stiffness, damping, mass, deltaTime);
        springStep(position.z, velocity.z, target.z, stiffness, damping, mass, deltaTime);
    }

    inline void springStep(float4& position, float4& velocity, const float4& target, float stiffness, float damping, float mass, float deltaTime)
    {
        springStep(position.x, velocity.x, target.x, stiffness, damping, mass, deltaTime);
        springStep(position.y, velocity.y, target.y, stiffness, damping, mass, deltaTime);
        springStep(position.z, velocity.z, target.z, stiffness, damping, mass, deltaTime);
        springStep(position.w, velocity.w, target.w, stiffness, damping, mass, deltaTime);
    }

    // Euclidean distance between two spring values, used to detect when a spring has settled
    // and to compute progress(). Overloaded per supported type.
    inline float springDistance(float a, float b) { return std::abs(b - a); }
    inline float springDistance(const float2& a, const float2& b) { return length(b - a); }
    inline float springDistance(const float3& a, const float3& b) { return std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y) + (b.z - a.z) * (b.z - a.z)); }
    inline float springDistance(const float4& a, const float4& b) { return std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y) + (b.z - a.z) * (b.z - a.z) + (b.w - a.w) * (b.w - a.w)); }

    template <typename T> bool isSpringSettled(const T& position, const T& velocity, const T& target)
    {
        constexpr float positionEpsilon = 0.01f;
        constexpr float velocityEpsilon = 0.01f;
        return springDistance(position, target) < positionEpsilon && springDistance(velocity, T {}) < velocityEpsilon;
    }
} // namespace p5::animation::detail

namespace p5::animation::detail
{
    template <typename T> T lerpValue(const T& from, const T& to, float t);
    template <> float2 lerpValue(const float2& from, const float2& to, float t);
    template <> float3 lerpValue(const float3& from, const float3& to, float t);
    template <> float4 lerpValue(const float4& from, const float4& to, float t);
    template <> color_t lerpValue(const color_t& from, const color_t& to, float t);
} // namespace p5::animation::detail

namespace p5::animation
{
    // clang-format off
    inline float easeLinear(float t) { return t; }
    inline float easeInQuad(float t) { return t * t; }
    inline float easeOutQuad(float t) { return t * (2.0f - t); }
    inline float easeInOutQuad(float t) { return (t < 0.5f) ? (2.0f * t * t) : (-1.0f + (4.0f - 2.0f * t) * t); }
    inline float easeInCubic(float t) { return t * t * t; }
    inline float easeOutCubic(float t) { const float u = t - 1.0f; return u * u * u + 1.0f; }
    inline float easeInOutCubic(float t) { return (t < 0.5f) ? (4.0f * t * t * t) : ((t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f); }
    inline float easeInQuart(float t) { return t * t * t * t; }
    inline float easeOutQuart(float t) { const float u = t - 1.0f; return 1.0f - u * u * u * u; }
    inline float easeInOutQuart(float t) { if (t < 0.5f) { return 8.0f * t * t * t * t; } const float u = t - 1.0f; return 1.0f - 8.0f * u * u * u * u; }
    inline float easeInQuint(float t) { return t * t * t * t * t; }
    inline float easeOutQuint(float t) { const float u = t - 1.0f; return 1.0f + u * u * u * u * u; }
    inline float easeInOutQuint(float t) { if (t < 0.5f) { return 16.0f * t * t * t * t * t; } const float u = t - 1.0f; return 1.0f + 16.0f * u * u * u * u * u; }
    inline float easeInSine(float t) { return 1.0f - std::cos((t * PI) / 2.0f); }
    inline float easeOutSine(float t) { return std::sin((t * PI) / 2.0f); }
    inline float easeInOutSine(float t) { return -(std::cos(PI * t) - 1.0f) / 2.0f; }
    inline float easeInExpo(float t) { return (t == 0.0f) ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f)); }
    inline float easeOutExpo(float t) { return (t == 1.0f) ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t); }
    inline float easeInOutExpo(float t) { return (t == 0.0f) ? 0.0f : (t == 1.0f) ? 1.0f : (t < 0.5f) ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f; }
    inline float easeInCirc(float t) { return 1.0f - std::sqrt(1.0f - t * t); }
    inline float easeOutCirc(float t) { const float u = t - 1.0f; return std::sqrt(1.0f - u * u); }
    inline float easeInOutCirc(float t) { if (t < 0.5f) { return (1.0f - std::sqrt(1.0f - 4.0f * t * t)) / 2.0f; } const float u = t - 1.0f; return (std::sqrt(1.0f - u * (2.0f * u)) + 1.0f) / 2.0f; }
    inline float easeInBack(float t) { const float s = 1.70158f; return t * t * ((s + 1.0f) * t - s); }
    inline float easeOutBack(float t) { const float s = 1.70158f; const float u = t - 1.0f; return u * u * ((s + 1.0f) * u + s) + 1.0f; }
    inline float easeInOutBack(float t) { const float s = 1.70158f * 1.525f; if (t < 0.5f) { return t * t * ((s + 1.0f) * 2.0f * t - s); } const float u = t - 1.0f; return u * u * ((s + 1.0f) * 2.0f * u + s) + 1.0f; }
    inline float easeInElastic(float t) { return (t == 0.0f) ? 0.0f : (t == 1.0f) ? 1.0f : -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * ((2.0f * PI) / 3.0f)); }
    inline float easeOutElastic(float t) { return (t == 0.0f) ? 0.0f : (t == 1.0f) ? 1.0f : std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * ((2.0f * PI) / 3.0f)) + 1.0f; }
    inline float easeInOutElastic(float t) { return (t == 0.0f) ? 0.0f : (t == 1.0f) ? 1.0f : (t < 0.5f) ? -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * ((2.0f * PI) / 4.5f))) / 2.0f : (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * ((2.0f * PI) / 4.5f))) / 2.0f + 1.0f; }
    inline float easeInBounce(float t) { return 1.0f - easeOutBounce(1.0f - t); }
    inline float easeOutBounce(float t) { if (t < 1.0f / 2.75f) { return 7.5625f * t * t; } else if (t < 2.0f / 2.75f) { t -= 1.5f / 2.75f; return 7.5625f * t * t + 0.75f; } else if (t < 2.5f / 2.75f) { t -= 2.25f / 2.75f; return 7.5625f * t * t + 0.9375f; } else { t -= 2.625f / 2.75f; return 7.5625f * t * t + 0.984375f; } }
    inline float easeInOutBounce(float t) { return (t < 0.5f) ? (1.0f - easeOutBounce(1.0f - 2.0f * t)) / 2.0f : (1.0f + easeOutBounce(2.0f * t - 1.0f)) / 2.0f; }
    // clang-format on
} // namespace p5::animation

namespace p5::animation
{
    template <typename T> tween<T> createTween(const T& from, const T& to, float duration, EasingFunction easing, LoopMode loopMode)
    {
        return tween<T> {
            .from = from,
            .to = to,
            .duration = duration,
            .easing = easing,
            .loopMode = loopMode,
            .repeatCount = -1,
            .elapsedTime = 0.0f,
            .isReversing = false,
            .state = PlayState::initial,
            .repeatsCompleted = 0,
        };
    }

    template <typename T> void restart(tween<T>& tween)
    {
        tween.state = PlayState::playing;
        tween.elapsedTime = 0.0f;
        tween.isReversing = false;
        tween.repeatsCompleted = 0;
    }

    template <typename T> void reset(tween<T>& tween)
    {
        tween.state = PlayState::initial;
        tween.elapsedTime = 0.0f;
        tween.isReversing = false;
        tween.repeatsCompleted = 0;
    }

    template <typename T> void pause(tween<T>& tween)
    {
        tween.state = PlayState::paused;
    }

    template <typename T> void resume(tween<T>& tween)
    {
        tween.state = PlayState::playing;
    }

    template <typename T> void advance(tween<T>& tween, float deltaTime)
    {
        if (tween.state != PlayState::playing) {
            return;
        }

        tween.elapsedTime += deltaTime;

        if (tween.elapsedTime < tween.duration) {
            return;
        }

        switch (tween.loopMode) {
            case LoopMode::once:
                tween.elapsedTime = tween.duration;
                tween.state = PlayState::finished;
                break;
            case LoopMode::loop: {
                const int cycles = (tween.duration > 0.0f) ? static_cast<int>(tween.elapsedTime / tween.duration) : 0;
                tween.elapsedTime = (tween.duration > 0.0f) ? std::fmod(tween.elapsedTime, tween.duration) : 0.0f;
                if (tween.repeatCount >= 0 && (tween.repeatsCompleted += cycles) >= tween.repeatCount) {
                    tween.elapsedTime = tween.duration;
                    tween.state = PlayState::finished;
                }
                break;
            }
            case LoopMode::pingpong: {
                const int cycles = (tween.duration > 0.0f) ? static_cast<int>(tween.elapsedTime / tween.duration) : 0;
                tween.elapsedTime = (tween.duration > 0.0f) ? std::fmod(tween.elapsedTime, tween.duration) : 0.0f;
                if (tween.repeatCount >= 0 && (tween.repeatsCompleted += cycles) >= tween.repeatCount) {
                    tween.state = PlayState::finished;
                    break;
                }
                if (cycles % 2 != 0) {
                    tween.isReversing = not tween.isReversing;
                }
                break;
            }
        }
    }

    template <typename T> void loop(tween<T>& tween, LoopMode loopMode, int repeatCount)
    {
        tween.loopMode = loopMode;
        tween.repeatCount = repeatCount;
    }

    template <typename T> void retarget(tween<T>& tween, const T& to)
    {
        tween.from = value(tween);
        tween.to = to;
        tween.elapsedTime = 0.0f;
        tween.isReversing = false;
        tween.state = PlayState::playing;
    }

    template <typename T> bool consumeFinished(tween<T>& tween)
    {
        if (not isFinished(tween)) {
            return false;
        }
        reset(tween);
        return true;
    }

    template <typename T> T value(const tween<T>& tween)
    {
        return detail::lerpValue(tween.from, tween.to, progress(tween));
    }

    template <typename T> float progress(const tween<T>& tween)
    {
        if (tween.duration <= 0.0f) {
            return 1.0f;
        }

        const float progress = std::clamp(tween.elapsedTime / tween.duration, 0.0f, 1.0f);
        const float easedProgress = tween.easing(progress);
        return tween.isReversing ? 1.0f - easedProgress : easedProgress;
    }

    template <typename T> bool isInitial(const tween<T>& tween) { return tween.state == PlayState::initial; }
    template <typename T> bool isPlaying(const tween<T>& tween) { return tween.state == PlayState::playing; }
    template <typename T> bool isPaused(const tween<T>& tween) { return tween.state == PlayState::paused; }
    template <typename T> bool isFinished(const tween<T>& tween) { return tween.state == PlayState::finished; }
} // namespace p5::animation

namespace p5::animation
{
    template <typename T> timeline<T> createTimeline(const T& from, const std::vector<timeline_entry<T>>& entries)
    {
        float totalDuration = 0.0f;
        for (const auto& entry : entries) {
            totalDuration += entry.duration;
        }

        return timeline<T> {
            .from = from,
            .totalDuration = totalDuration,
            .entries = entries,
            .elapsedTime = 0.0f,
            .loopMode = LoopMode::once,
            .repeatCount = -1,
            .isReversing = false,
            .state = PlayState::initial,
            .repeatsCompleted = 0,
        };
    }

    template <typename T> void restart(timeline<T>& timeline)
    {
        timeline.state = PlayState::playing;
        timeline.elapsedTime = 0.0f;
        timeline.isReversing = false;
        timeline.repeatsCompleted = 0;
    }

    template <typename T> void reset(timeline<T>& timeline)
    {
        timeline.state = PlayState::initial;
        timeline.elapsedTime = 0.0f;
        timeline.isReversing = false;
        timeline.repeatsCompleted = 0;
    }

    template <typename T> void pause(timeline<T>& timeline)
    {
        timeline.state = PlayState::paused;
    }

    template <typename T> void resume(timeline<T>& timeline)
    {
        timeline.state = PlayState::playing;
    }

    template <typename T> void advance(timeline<T>& timeline, float deltaTime)
    {
        if (timeline.state != PlayState::playing) {
            return;
        }

        timeline.elapsedTime += deltaTime;

        if (timeline.elapsedTime < timeline.totalDuration) {
            return;
        }

        switch (timeline.loopMode) {
            case LoopMode::once:
                timeline.elapsedTime = timeline.totalDuration;
                timeline.state = PlayState::finished;
                break;
            case LoopMode::loop: {
                const int cycles = (timeline.totalDuration > 0.0f) ? static_cast<int>(timeline.elapsedTime / timeline.totalDuration) : 0;
                timeline.elapsedTime = (timeline.totalDuration > 0.0f) ? std::fmod(timeline.elapsedTime, timeline.totalDuration) : 0.0f;
                if (timeline.repeatCount >= 0 && (timeline.repeatsCompleted += cycles) >= timeline.repeatCount) {
                    timeline.elapsedTime = timeline.totalDuration;
                    timeline.state = PlayState::finished;
                }
                break;
            }
            case LoopMode::pingpong: {
                const int cycles = (timeline.totalDuration > 0.0f) ? static_cast<int>(timeline.elapsedTime / timeline.totalDuration) : 0;
                timeline.elapsedTime = (timeline.totalDuration > 0.0f) ? std::fmod(timeline.elapsedTime, timeline.totalDuration) : 0.0f;
                if (timeline.repeatCount >= 0 && (timeline.repeatsCompleted += cycles) >= timeline.repeatCount) {
                    timeline.state = PlayState::finished;
                    break;
                }
                if (cycles % 2 != 0) {
                    timeline.isReversing = not timeline.isReversing;
                }
                break;
            }
        }
    }

    template <typename T> void loop(timeline<T>& timeline, LoopMode loopMode, int repeatCount)
    {
        timeline.loopMode = loopMode;
        timeline.repeatCount = repeatCount;
    }

    template <typename T> bool consumeFinished(timeline<T>& timeline)
    {
        if (not isFinished(timeline)) {
            return false;
        }
        reset(timeline);
        return true;
    }

    template <typename T> T value(const timeline<T>& timeline)
    {
        if (timeline.entries.empty()) {
            return timeline.from;
        }

        float elapsedTime = timeline.isReversing ? (timeline.totalDuration - timeline.elapsedTime) : timeline.elapsedTime;
        size_t entryIndex = 0;

        while (entryIndex < timeline.entries.size() && elapsedTime >= timeline.entries[entryIndex].duration) {
            elapsedTime -= timeline.entries[entryIndex].duration;
            ++entryIndex;
        }

        if (entryIndex >= timeline.entries.size()) {
            return timeline.entries.back().to;
        }

        const auto& entry = timeline.entries[entryIndex];
        const float progress = std::clamp(elapsedTime / entry.duration, 0.0f, 1.0f);
        const float easedProgress = entry.easing(progress);

        if (entryIndex == 0) {
            return detail::lerpValue(timeline.from, entry.to, easedProgress);
        }

        const timeline_entry<T>& previousEntry = timeline.entries[entryIndex - 1];
        return detail::lerpValue(previousEntry.to, entry.to, easedProgress);
    }

    template <typename T> float progress(const timeline<T>& timeline)
    {
        if (timeline.totalDuration <= 0.0f) {
            return 1.0f;
        }

        const float progress = std::clamp(timeline.elapsedTime / timeline.totalDuration, 0.0f, 1.0f);
        return timeline.isReversing ? 1.0f - progress : progress;
    }

    template <typename T> bool isInitial(const timeline<T>& timeline) { return timeline.state == PlayState::initial; }
    template <typename T> bool isPlaying(const timeline<T>& timeline) { return timeline.state == PlayState::playing; }
    template <typename T> bool isPaused(const timeline<T>& timeline) { return timeline.state == PlayState::paused; }
    template <typename T> bool isFinished(const timeline<T>& timeline) { return timeline.state == PlayState::finished; }
} // namespace p5::animation

namespace p5::animation
{
    template <typename T> spring<T> createSpring(const T& from, const T& to, float stiffness, float damping, float mass, LoopMode loopMode)
    {
        return spring<T> {
            .from = from,
            .to = to,
            .stiffness = stiffness,
            .damping = damping,
            .mass = mass,
            .loopMode = loopMode,
            .repeatCount = -1,
            .velocity = T {},
            .position = from,
            .state = PlayState::initial,
            .isReversing = false,
            .repeatsCompleted = 0,
        };
    }

    template <typename T> void restart(spring<T>& spring)
    {
        spring.state = PlayState::playing;
        spring.position = spring.from;
        spring.velocity = T {};
        spring.isReversing = false;
        spring.repeatsCompleted = 0;
    }

    template <typename T> void reset(spring<T>& spring)
    {
        spring.state = PlayState::initial;
        spring.position = spring.from;
        spring.velocity = T {};
        spring.isReversing = false;
        spring.repeatsCompleted = 0;
    }

    template <typename T> void pause(spring<T>& spring)
    {
        spring.state = PlayState::paused;
    }

    template <typename T> void resume(spring<T>& spring)
    {
        spring.state = PlayState::playing;
    }

    template <typename T> void advance(spring<T>& spring, float deltaTime)
    {
        if (spring.state != PlayState::playing) {
            return;
        }

        const T& target = spring.isReversing ? spring.from : spring.to;

        detail::springStep(spring.position, spring.velocity, target, spring.stiffness, spring.damping, spring.mass, deltaTime);

        if (not detail::isSpringSettled(spring.position, spring.velocity, target)) {
            return;
        }

        spring.position = target;
        spring.velocity = T {};

        switch (spring.loopMode) {
            case LoopMode::once:
                spring.state = PlayState::finished;
                break;
            case LoopMode::loop:
                if (spring.repeatCount >= 0 && ++spring.repeatsCompleted >= spring.repeatCount) {
                    spring.state = PlayState::finished;
                    break;
                }
                spring.position = spring.from;
                break;
            case LoopMode::pingpong:
                if (spring.repeatCount >= 0 && ++spring.repeatsCompleted >= spring.repeatCount) {
                    spring.state = PlayState::finished;
                    break;
                }
                spring.isReversing = !spring.isReversing;
                break;
        }
    }

    template <typename T> void loop(spring<T>& spring, LoopMode loopMode, int repeatCount)
    {
        spring.loopMode = loopMode;
        spring.repeatCount = repeatCount;
    }

    template <typename T> bool consumeFinished(spring<T>& spring)
    {
        if (not isFinished(spring)) {
            return false;
        }
        reset(spring);
        return true;
    }

    template <typename T> T value(const spring<T>& spring) { return spring.position; }
    template <typename T> float progress(const spring<T>& spring)
    {
        const T& target = spring.isReversing ? spring.from : spring.to;
        const T& origin = spring.isReversing ? spring.to : spring.from;

        const float totalDistance = detail::springDistance(origin, target);
        if (totalDistance <= 0.0f) {
            return 1.0f;
        }

        const float remainingDistance = detail::springDistance(spring.position, target);
        return std::clamp(1.0f - remainingDistance / totalDistance, 0.0f, 1.0f);
    }

    template <typename T> bool isInitial(const spring<T>& spring) { return spring.state == PlayState::initial; }
    template <typename T> bool isPlaying(const spring<T>& spring) { return spring.state == PlayState::playing; }
    template <typename T> bool isPaused(const spring<T>& spring) { return spring.state == PlayState::paused; }
    template <typename T> bool isFinished(const spring<T>& spring) { return spring.state == PlayState::finished; }
} // namespace p5::animation

namespace p5::animation
{
    inline sequence createSequence(const std::vector<sequence_step>& steps)
    {
        return sequence {
            .steps = steps,
            .stepIndex = 0,
            .stepElapsedTime = 0.0f,
            .stepEntered = false,
            .loopMode = LoopMode::once,
            .repeatCount = -1,
            .state = PlayState::initial,
            .repeatsCompleted = 0,
        };
    }

    inline void restart(sequence& sequence)
    {
        sequence.state = PlayState::playing;
        sequence.stepIndex = 0;
        sequence.stepElapsedTime = 0.0f;
        sequence.stepEntered = false;
        sequence.repeatsCompleted = 0;
    }

    inline void reset(sequence& sequence)
    {
        sequence.state = PlayState::initial;
        sequence.stepIndex = 0;
        sequence.stepElapsedTime = 0.0f;
        sequence.stepEntered = false;
        sequence.repeatsCompleted = 0;
    }

    inline void pause(sequence& sequence)
    {
        sequence.state = PlayState::paused;
    }

    inline void resume(sequence& sequence)
    {
        sequence.state = PlayState::playing;
    }

    inline void advance(sequence& sequence, float deltaTime)
    {
        if (sequence.state != PlayState::playing) {
            return;
        }

        if (sequence.steps.empty()) {
            sequence.state = PlayState::finished;
            return;
        }

        const size_t stepCount = sequence.steps.size();

        // Bounded to stepCount+1 transitions per call: lets a chain of zero-duration steps (call())
        // resolve within a single frame without consuming deltaTime, while still guaranteeing this
        // loop terminates even if every step in the sequence happens to be instantaneous.
        for (size_t transitions = 0; sequence.state == PlayState::playing && transitions <= stepCount; ++transitions) {
            sequence_step& step = sequence.steps[sequence.stepIndex];

            if (not sequence.stepEntered) {
                sequence.stepEntered = true;
                if (step.onEnter) {
                    step.onEnter();
                }
            }

            const bool done = step.isDone ? step.isDone() : (sequence.stepElapsedTime >= step.duration);
            if (not done) {
                if (step.onAdvance) {
                    step.onAdvance(deltaTime);
                }
                sequence.stepElapsedTime += deltaTime;
                break;
            }

            ++sequence.stepIndex;
            sequence.stepElapsedTime = 0.0f;
            sequence.stepEntered = false;

            if (sequence.stepIndex < stepCount) {
                continue;
            }

            sequence.stepIndex = 0;
            if (sequence.repeatCount >= 0 && ++sequence.repeatsCompleted >= sequence.repeatCount) {
                sequence.state = PlayState::finished;
                break;
            }
            if (sequence.loopMode == LoopMode::once) {
                sequence.state = PlayState::finished;
                break;
            }
            // loop: restart from the first step. pingpong cannot reach here - loop() rejects it.
        }
    }

    inline void loop(sequence& sequence, LoopMode loopMode, int repeatCount)
    {
        assert(loopMode != LoopMode::pingpong && "sequence steps are one-shot side effects and cannot be played backwards - use LoopMode::once or LoopMode::loop (with an optional repeatCount) instead");
        sequence.loopMode = loopMode;
        sequence.repeatCount = repeatCount;
    }

    inline bool isInitial(const sequence& sequence) { return sequence.state == PlayState::initial; }
    inline bool isPlaying(const sequence& sequence) { return sequence.state == PlayState::playing; }
    inline bool isPaused(const sequence& sequence) { return sequence.state == PlayState::paused; }
    inline bool isFinished(const sequence& sequence) { return sequence.state == PlayState::finished; }

    inline bool consumeFinished(sequence& sequence)
    {
        if (not isFinished(sequence)) {
            return false;
        }
        reset(sequence);
        return true;
    }

    inline size_t currentStep(const sequence& sequence)
    {
        return sequence.steps.empty() ? 0 : sequence.stepIndex;
    }

    inline sequence_step wait(float seconds)
    {
        return sequence_step {.duration = seconds};
    }

    inline sequence_step call(std::function<void()> action)
    {
        return sequence_step {
            .onEnter = std::move(action),
            .isDone = [] { return true; },
        };
    }

    template <typename T> sequence_step play(tween<T>& tween)
    {
        return sequence_step {
            .onEnter = [&tween] { restart(tween); },
            .onAdvance = [&tween](float dt) { advance(tween, dt); },
            .isDone = [&tween] { return isFinished(tween); },
        };
    }

    template <typename T> sequence_step play(spring<T>& spring)
    {
        return sequence_step {
            .onEnter = [&spring] { restart(spring); },
            .onAdvance = [&spring](float dt) { advance(spring, dt); },
            .isDone = [&spring] { return isFinished(spring); },
        };
    }

    template <typename T> sequence_step play(timeline<T>& timeline)
    {
        return sequence_step {
            .onEnter = [&timeline] { restart(timeline); },
            .onAdvance = [&timeline](float dt) { advance(timeline, dt); },
            .isDone = [&timeline] { return isFinished(timeline); },
        };
    }
} // namespace p5::animation

namespace p5::animation
{
    template <typename T> T detail::lerpValue(const T& from, const T& to, float t) { return from + (to - from) * t; }
    template <> inline float2 detail::lerpValue(const float2& from, const float2& to, float t) { return {lerpValue(from.x, to.x, t), lerpValue(from.y, to.y, t)}; }
    template <> inline float3 detail::lerpValue(const float3& from, const float3& to, float t) { return {lerpValue(from.x, to.x, t), lerpValue(from.y, to.y, t), lerpValue(from.z, to.z, t)}; }
    template <> inline float4 detail::lerpValue(const float4& from, const float4& to, float t) { return {lerpValue(from.x, to.x, t), lerpValue(from.y, to.y, t), lerpValue(from.z, to.z, t), lerpValue(from.w, to.w, t)}; }
    template <> inline color_t detail::lerpValue(const color_t& from, const color_t& to, float t)
    {
        const uint8_t r = static_cast<uint8_t>(lerpValue(getRed(from), getRed(to), t));
        const uint8_t g = static_cast<uint8_t>(lerpValue(getGreen(from), getGreen(to), t));
        const uint8_t b = static_cast<uint8_t>(lerpValue(getBlue(from), getBlue(to), t));
        const uint8_t a = static_cast<uint8_t>(lerpValue(getAlpha(from), getAlpha(to), t));

        return rgba(r, g, b, a);
    }
} // namespace p5::animation
