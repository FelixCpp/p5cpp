#include "timing.hpp"

#include <thread>

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

namespace p5
{

    // ── precise_sleep_until ───────────────────────────────────────────────────────

#ifdef __APPLE__

    // Convert a duration in nanoseconds to Mach absolute time units.
    // The timebase ratio (numer/denom) is queried once and cached.
    static uint64_t nanosToMachTime(uint64_t ns)
    {
        static const mach_timebase_info_data_t tb = [] {
            mach_timebase_info_data_t info;
            mach_timebase_info(&info);
            return info;
        }();

        // Use __uint128_t to avoid overflow when ns is large.
        return static_cast<uint64_t>(
            static_cast<__uint128_t>(ns) * tb.denom / tb.numer
        );
    }

    // Returns the mach_absolute_time that corresponds to the given steady_clock
    // time point.  We anchor the conversion once at first call by sampling both
    // clocks simultaneously and storing the offset.
    static uint64_t machTimeFromTimePoint(TimePoint tp)
    {
        struct Anchor
        {
            uint64_t machTime;
            TimePoint chronoTime;
        };

        static const Anchor anchor = [] {
            // Sample both clocks as close together as possible.
            const uint64_t m = mach_absolute_time();
            const TimePoint c = Clock::now();
            return Anchor {m, c};
        }();

        const auto deltaNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                 tp - anchor.chronoTime
        )
                                 .count();

        if (deltaNs >= 0)
            return anchor.machTime + nanosToMachTime(static_cast<uint64_t>(deltaNs));

        // Target is in the past relative to the anchor — clamp to zero delta.
        const uint64_t absDeltaNs = static_cast<uint64_t>(-deltaNs);
        const uint64_t machDelta = nanosToMachTime(absDeltaNs);
        return (machDelta <= anchor.machTime) ? anchor.machTime - machDelta : 0;
    }

    void precise_sleep_until(TimePoint target)
    {
        mach_wait_until(machTimeFromTimePoint(target));
    }

#else

    void precise_sleep_until(TimePoint target)
    {
        std::this_thread::sleep_until(target);
    }

#endif // __APPLE__

    // ── FpsCounter ────────────────────────────────────────────────────────────────

    FpsCounter::FpsCounter(float windowSeconds)
        : m_windowSeconds(windowSeconds)
    {
    }

    void FpsCounter::tick()
    {
        ++m_frames;

        const TimePoint now = Clock::now();
        const float elapsed = std::chrono::duration<float>(now - m_windowStart).count();

        if (elapsed >= m_windowSeconds) {
            m_current = static_cast<float>(m_frames) / elapsed;
            m_frames = 0;
            m_windowStart = now;
        }
    }

    float FpsCounter::fps() const
    {
        return m_current;
    }
} // namespace p5
