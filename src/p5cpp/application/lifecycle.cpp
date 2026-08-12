#include <p5cpp/application/lifecycle.hpp>

namespace p5
{
    Lifecycle::Lifecycle()
        : m_shouldClose {false},
          m_shouldRestart {false},
          m_exitCode {0},
          m_frameCount {0}
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
        ++m_frameCount;
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
} // namespace p5
