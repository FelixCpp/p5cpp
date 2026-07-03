#pragma once

#include <chrono>

namespace p5cpp
{
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    // Blocks the calling thread until `target`, using a platform high-resolution
    // wait primitive.  Falls back to std::this_thread::sleep_until on platforms
    // where no such primitive is available.
    void precise_sleep_until(TimePoint target);
} // namespace p5cpp
