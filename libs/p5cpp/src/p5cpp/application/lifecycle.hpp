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

        void loop();
        void noLoop();
        void redraw();
        void setFrameRate(double framesPerSecond);
        void quitOnEscape(bool enabled);

        bool shouldClose() const;
        bool shouldRestart() const;
        int getExitCode() const;
        int frameCount() const;
        double deltaTime() const;
        double globalTime() const;
        bool isLooping() const;
        bool shouldDrawThisFrame() const;
        double frameRate() const;
        double measuredFrameRate() const;
        bool quitsOnEscape() const;

    private:
        void waitForFrameDeadline();

    private:
        bool m_shouldClose;
        bool m_shouldRestart;
        bool m_looping;
        bool m_redrawRequested;
        bool m_drawingThisFrame;
        bool m_quitOnEscape;
        int m_exitCode;
        int m_frameCount;

        std::chrono::steady_clock::time_point m_startTime;
        std::chrono::steady_clock::time_point m_lastFrameTime;
        double m_deltaTime;
        double m_globalTime;

        double m_targetFrameRate;
        double m_measuredFrameRate;
        bool m_hasFrameDeadline;
        std::chrono::steady_clock::time_point m_frameDeadline;
    };
} // namespace p5
