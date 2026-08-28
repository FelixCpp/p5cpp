#include <p5cpp/application/lifecycle.hpp>

namespace p5
{
    Lifecycle::Lifecycle()
        : m_shouldClose {false},
          m_shouldRestart {false},
          m_looping {true},
          m_redrawRequested {false},
          m_drawingThisFrame {true},
          m_exitCode {0},
          m_frameCount {0},
          m_startTime {std::chrono::steady_clock::now()},
          m_lastFrameTime {m_startTime},
          m_deltaTime {0.0},
          m_globalTime {0.0}
    {
    }

    void Lifecycle::exitCode(int exitCode)
    {
        m_exitCode = exitCode;
    }

    void Lifecycle::close(int exitCode)
    {
        m_exitCode = exitCode;
        m_shouldClose = true;
    }

    void Lifecycle::close()
    {
        m_shouldClose = true;
    }

    void Lifecycle::restart()
    {
        m_shouldRestart = true;
        m_shouldClose = true;
    }

    void Lifecycle::nextFrame()
    {
        m_drawingThisFrame = m_looping or m_redrawRequested;
        m_redrawRequested = false;

        if (not m_drawingThisFrame) {
            return;
        }

        ++m_frameCount;

        const auto now = std::chrono::steady_clock::now();
        m_deltaTime = std::chrono::duration<double>(now - m_lastFrameTime).count();
        m_globalTime = std::chrono::duration<double>(now - m_startTime).count();
        m_lastFrameTime = now;
    }

    void Lifecycle::loop()
    {
        m_looping = true;
    }

    void Lifecycle::noLoop()
    {
        m_looping = false;
    }

    void Lifecycle::redraw()
    {
        m_redrawRequested = true;
    }

    bool Lifecycle::shouldClose() const
    {
        return m_shouldClose;
    }

    bool Lifecycle::shouldRestart() const
    {
        return m_shouldRestart;
    }

    int Lifecycle::getExitCode() const
    {
        return m_exitCode;
    }

    int Lifecycle::frameCount() const
    {
        return m_frameCount;
    }

    double Lifecycle::deltaTime() const
    {
        return m_deltaTime;
    }

    double Lifecycle::globalTime() const
    {
        return m_globalTime;
    }

    bool Lifecycle::isLooping() const
    {
        return m_looping;
    }

    bool Lifecycle::shouldDrawThisFrame() const
    {
        return m_drawingThisFrame;
    }
} // namespace p5
