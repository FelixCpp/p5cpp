#pragma once

#include <chrono>

namespace p5cpp
{
    class FrameComponent
    {
    public:
        FrameComponent();

        void update();

        void loop();
        void noLoop();

        void frameRate(int frameRate);
        void quit();
        void quit(int exitCode);
        void exitCode(int exitCode);
        void restart();

        bool isLooping() const;
        int getFrameCount() const;
        int getFrameRate() const;
        float getDeltaTime() const;
        float getGlobalTime() const;
        bool isCloseRequested() const;
        bool isRestartRequested() const;

    private:
        float m_deltaTime;
        float m_globalTime;
        float m_framesPerSecond;
        uint64_t m_frameCount;

        int m_targetFrameRate;

        bool m_closeRequested;
        bool m_restartRequested;
        bool m_isPaused;
        int m_exitCode;

        float m_fpsCalculationInterval;
        int m_framesPerCalculation;
        std::chrono::steady_clock::time_point m_lastCalculationTimestamp;
        std::chrono::steady_clock::time_point m_frameStartTimestamp;
        std::chrono::steady_clock::time_point m_lastFrameStart;
    };
} // namespace p5cpp
