#pragma once

#include <cstdint>

namespace p5cpp
{
    struct FrameData
    {
        float deltaTime;
        float globalTime;
        float framesPerSecond;
        uint64_t frameCount;

        int targetFrameRate;

        bool closeRequested;
        bool isPaused;
        int exitCode;
    };
} // namespace p5cpp
