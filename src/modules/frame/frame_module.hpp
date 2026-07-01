#pragma once

#include "../../module.hpp"
#include "frame_data.hpp"

#include <chrono>

namespace p5cpp
{
    class FrameModule : public Module
    {
    public:
        void setup(AppContext& context, Next next) override;
        void event(AppContext& context, WindowEvent& event, Next next) override;
        void draw(AppContext& context, Next next) override;
        void destroy(AppContext& context, Next next) override;

    private:
        FrameData data;

        float fpsCalculationInterval;
        int framesPerCalculation;
        std::chrono::steady_clock::time_point lastCalculationTimestamp;
        std::chrono::steady_clock::time_point frameStartTimestamp;
        std::chrono::steady_clock::time_point lastFrameStart;
    };
} // namespace p5cpp
