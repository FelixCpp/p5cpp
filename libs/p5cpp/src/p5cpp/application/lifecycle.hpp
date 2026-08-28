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
        // Resolves whether this frame should actually draw (isLooping(), or a pending redraw()
        // request), consuming that pending request, and -- only when drawing -- advances
        // frameCount/deltaTime/globalTime. Called once per frame from Kernel::run(); the result is
        // read back via shouldDrawThisFrame() for the rest of that frame's dispatch chain.
        void nextFrame();

        void loop();
        void noLoop();
        // Requests a single extra draw() call for the next frame even while noLoop() is active.
        // No-op while already looping. Consumed (and cleared) by the next nextFrame().
        void redraw();

        bool shouldClose() const;
        bool shouldRestart() const;
        int getExitCode() const;
        int frameCount() const;
        double deltaTime() const;
        double globalTime() const;
        bool isLooping() const;
        bool shouldDrawThisFrame() const;

    private:
        bool m_shouldClose;
        bool m_shouldRestart;
        bool m_looping;
        bool m_redrawRequested;
        bool m_drawingThisFrame;
        int m_exitCode;
        int m_frameCount;

        std::chrono::steady_clock::time_point m_startTime;
        std::chrono::steady_clock::time_point m_lastFrameTime;
        double m_deltaTime;
        double m_globalTime;
    };
} // namespace p5
