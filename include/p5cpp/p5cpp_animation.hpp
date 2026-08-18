#pragma once

#include <p5cpp/p5cpp.hpp>

#include <algorithm>

namespace p5
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
} // namespace p5

namespace p5
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
} // namespace p5

namespace p5
{
    template <typename T> struct tween
    {
        T from;                // Starting value of the tween
        T to;                  // Destination value of the tween
        float duration;        // Duration of the tween in seconds
        EasingFunction easing; // Easing function to apply to the tween
        LoopMode loopMode;     // Looping behavior of the tween

        float elapsedTime; // Elapsed time since the start of the tween in seconds
        bool isReversing;  // Flag indicating whether the tween is currently reversing (for pingpong mode)
        PlayState state;   // Current state of the tween (playing, paused, or stopped)
    };

    template <typename T> tween<T> createTween(const T& from, const T& to, float duration, EasingFunction easing = &easeInOutSine, LoopMode loopMode = LoopMode::once);
    template <typename T> void restart(tween<T>& tween);
    template <typename T> void reset(tween<T>& tween);
    template <typename T> void pause(tween<T>& tween);
    template <typename T> void resume(tween<T>& tween);
    template <typename T> void advance(tween<T>& tween, float deltaTime);
    template <typename T> void loop(tween<T>& tween, LoopMode loopMode);

    template <typename T> T value(const tween<T>& tween);
    template <typename T> float progress(const tween<T>& tween);
    template <typename T> bool isInitial(const tween<T>& tween);
    template <typename T> bool isPlaying(const tween<T>& tween);
    template <typename T> bool isPaused(const tween<T>& tween);
    template <typename T> bool isFinished(const tween<T>& tween);
} // namespace p5

namespace p5
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
        bool isReversing;
        PlayState state;
        size_t currentEntryIndex;
    };

    template <typename T> timeline<T> createTimeline(const T& from, const std::vector<timeline_entry<T>>& entries);
    template <typename T> void restart(timeline<T>& timeline);
    template <typename T> void reset(timeline<T>& timeline);
    template <typename T> void pause(timeline<T>& timeline);
    template <typename T> void resume(timeline<T>& timeline);
    template <typename T> void advance(timeline<T>& timeline, float deltaTime);

    template <typename T> T value(const timeline<T>& timeline);
} // namespace p5

namespace p5::detail
{
    template <typename T> T lerpValue(const T& from, const T& to, float t);
    template <> float2 lerpValue(const float2& from, const float2& to, float t);
    template <> float3 lerpValue(const float3& from, const float3& to, float t);
    template <> float4 lerpValue(const float4& from, const float4& to, float t);
    template <> color_t lerpValue(const color_t& from, const color_t& to, float t);
} // namespace p5::detail

namespace p5
{
    // clang-format off
    inline float easeLinear(float t) { return t; }
    inline float easeInQuad(float t) { return t * t; }
    inline float easeOutQuad(float t) { return t * (2.0f - t); }
    inline float easeInOutQuad(float t) { return (t < 0.5f) ? (2.0f * t * t) : (-1.0f + (4.0f - 2.0f * t) * t); }
    inline float easeInCubic(float t) { return t * t * t; }
    inline float easeOutCubic(float t) { return (--t) * t * t + 1.0f; }
    inline float easeInOutCubic(float t) { return (t < 0.5f) ? (4.0f * t * t * t) : ((t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f); }
    inline float easeInQuart(float t) { return t * t * t * t; }
    inline float easeOutQuart(float t) { return 1.0f - (--t) * t * t * t; }
    inline float easeInOutQuart(float t) { return (t < 0.5f) ? (8.0f * t * t * t * t) : (1.0f - 8.0f * (--t) * t * t * t); }
    inline float easeInQuint(float t) { return t * t * t * t * t; }
    inline float easeOutQuint(float t) { return 1.0f + (--t) * t * t * t * t; }
    inline float easeInOutQuint(float t) { return (t < 0.5f) ? (16.0f * t * t * t * t * t) : (1.0f + 16.0f * (--t) * t * t * t * t); }
    inline float easeInSine(float t) { return 1.0f - std::cos((t * PI) / 2.0f); }
    inline float easeOutSine(float t) { return std::sin((t * PI) / 2.0f); }
    inline float easeInOutSine(float t) { return -(std::cos(PI * t) - 1.0f) / 2.0f; }
    inline float easeInExpo(float t) { return (t == 0.0f) ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f)); }
    inline float easeOutExpo(float t) { return (t == 1.0f) ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t); }
    inline float easeInOutExpo(float t) { return (t == 0.0f) ? 0.0f : (t == 1.0f) ? 1.0f : (t < 0.5f) ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f; }
    inline float easeInCirc(float t) { return 1.0f - std::sqrt(1.0f - t * t); }
    inline float easeOutCirc(float t) { return std::sqrt(1.0f - (--t) * t); }
    inline float easeInOutCirc(float t) { return (t < 0.5f) ? (1.0f - std::sqrt(1.0f - 4.0f * t * t)) / 2.0f : (std::sqrt(1.0f - (--t) * (2.0f * t)) + 1.0f) / 2.0f; }
    inline float easeInBack(float t) { const float s = 1.70158f; return t * t * ((s + 1.0f) * t - s); }
    inline float easeOutBack(float t) { const float s = 1.70158f; return (--t) * t * ((s + 1.0f) * t + s) + 1.0f; }
    inline float easeInOutBack(float t) { const float s = 1.70158f * 1.525f; return (t < 0.5f) ? (t * t * ((s + 1.0f) * 2.0f * t - s)) : ((--t) * t * ((s + 1.0f) * 2.0f * t + s) + 1.0f); }
    inline float easeInElastic(float t) { return (t == 0.0f) ? 0.0f : (t == 1.0f) ? 1.0f : -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * ((2.0f * PI) / 3.0f)); }
    inline float easeOutElastic(float t) { return (t == 0.0f) ? 0.0f : (t == 1.0f) ? 1.0f : std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * ((2.0f * PI) / 3.0f)) + 1.0f; }
    inline float easeInOutElastic(float t) { return (t == 0.0f) ? 0.0f : (t == 1.0f) ? 1.0f : (t < 0.5f) ? -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * ((2.0f * PI) / 4.5f))) / 2.0f : (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * ((2.0f * PI) / 4.5f))) / 2.0f + 1.0f; }
    inline float easeInBounce(float t) { return 1.0f - easeOutBounce(1.0f - t); }
    inline float easeOutBounce(float t) { if (t < 1.0f / 2.75f) { return 7.5625f * t * t; } else if (t < 2.0f / 2.75f) { t -= 1.5f / 2.75f; return 7.5625f * t * t + 0.75f; } else if (t < 2.5f / 2.75f) { t -= 2.25f / 2.75f; return 7.5625f * t * t + 0.9375f; } else { t -= 2.625f / 2.75f; return 7.5625f * t * t + 0.984375f; } }
    inline float easeInOutBounce(float t) { return (t < 0.5f) ? (1.0f - easeOutBounce(1.0f - 2.0f * t)) / 2.0f : (1.0f + easeOutBounce(2.0f * t - 1.0f)) / 2.0f; }
    // clang-format on
} // namespace p5

namespace p5
{
    template <typename T> tween<T> createTween(const T& from, const T& to, float duration, EasingFunction easing, LoopMode loopMode)
    {
        return tween<T> {
            .from = from,
            .to = to,
            .duration = duration,
            .easing = easing,
            .loopMode = loopMode,
            .elapsedTime = 0.0f,
            .isReversing = false,
            .state = PlayState::initial,
        };
    }

    template <typename T> void restart(tween<T>& tween)
    {
        tween.state = PlayState::playing;
        tween.elapsedTime = 0.0f;
        tween.isReversing = false;
    }

    template <typename T> void reset(tween<T>& tween)
    {
        tween.state = PlayState::initial;
        tween.elapsedTime = 0.0f;
        tween.isReversing = false;
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
            case LoopMode::loop:
                tween.elapsedTime = (tween.duration > 0.0f) ? std::fmod(tween.elapsedTime, tween.duration) : 0.0f;
                break;
            case LoopMode::pingpong: {
                const int cycles = (tween.duration > 0.0f) ? static_cast<int>(tween.elapsedTime / tween.duration) : 0;
                tween.elapsedTime = (tween.duration > 0.0f) ? std::fmod(tween.elapsedTime, tween.duration) : 0.0f;
                if (cycles % 2 != 0) {
                    tween.isReversing = not tween.isReversing;
                }
                break;
            }
        }
    }

    template <typename T> void loop(tween<T>& tween, LoopMode loopMode) { tween.loopMode = loopMode; }

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
} // namespace p5

namespace p5
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
} // namespace p5
