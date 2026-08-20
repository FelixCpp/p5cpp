#pragma once

#include <chrono>

namespace p5
{
    class Lifecycle
    {
    public:
        Lifecycle();

        void exitCode(int exitCode);
        void close(int exitCode);
        void close();
        void restart();
        void nextFrame();

        bool shouldClose() const;
        bool shouldRestart() const;
        int getExitCode() const;
        int frameCount() const;
        double deltaTime() const;
        double globalTime() const;

    private:
        bool m_shouldClose;
        bool m_shouldRestart;
        int m_exitCode;
        int m_frameCount;

        std::chrono::steady_clock::time_point m_startTime;
        std::chrono::steady_clock::time_point m_lastFrameTime;
        double m_deltaTime;
        double m_globalTime;
    };
} // namespace p5
