#pragma once

#include "../module.hpp"

#include <chrono>

namespace p5cpp
{
    class LifecycleModule : public Module
    {
    public:
        void setup(AppContext& context, Next next) override;
        void event(AppContext& context, WindowEvent& event, Next next) override;
        void draw(AppContext& context, Next next) override;

    private:
        float fpsCalculationInterval;
        int framesPerCalculation;
        std::chrono::steady_clock::time_point lastCalculationTimestamp;
        std::chrono::steady_clock::time_point frameStartTimestamp;
        std::chrono::steady_clock::time_point lastFrameStart;
    };
} // namespace p5cpp
