#pragma once

#include <chrono>

namespace p5cpp
{
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    void precise_sleep_until(TimePoint target);
} // namespace p5cpp
