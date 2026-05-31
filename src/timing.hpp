#pragma once

#include <chrono>
#include <cstdint>

namespace p5
{
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    // Blocks the calling thread until `target`, using a platform high-resolution
    // wait primitive.  Falls back to std::this_thread::sleep_until on platforms
    // where no such primitive is available.
    void precise_sleep_until(TimePoint target);

    // Opaque per-frame FPS tracker.  Call tick() once per rendered frame;
    // fps() returns the average over the last full measurement window.
    class FpsCounter
    {
    public:
        explicit FpsCounter(float windowSeconds = 1.0f);

        // Call once after every successfully rendered frame.
        void tick();

        // Returns the most recently completed frames-per-second measurement.
        // Returns 0 until the first full window has elapsed.
        float fps() const;

    private:
        float     m_windowSeconds;
        float     m_current        = 0.0f;
        int       m_frames         = 0;
        TimePoint m_windowStart    = Clock::now();
    };

} // namespace p5
