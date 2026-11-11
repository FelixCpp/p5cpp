#include <p5cpp/application/lifecycle.hpp>

#include <thread>

namespace p5
{
    namespace
    {
        void sleepUntil(std::chrono::steady_clock::time_point deadline)
        {
            constexpr auto spinThreshold = std::chrono::microseconds {1500};

            while (true) {
                const auto now = std::chrono::steady_clock::now();

                if (now >= deadline) {
                    return;
                }

                if (const auto remaining = deadline - now; remaining > spinThreshold) {
                    std::this_thread::sleep_for(remaining - spinThreshold);
                } else {
                    std::this_thread::yield();
                }
            }
        }
    } // namespace

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
          m_globalTime {0.0},
          m_targetFrameRate {0.0},
          m_measuredFrameRate {0.0},
          m_hasFrameDeadline {false},
          m_frameDeadline {m_startTime}
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

        waitForFrameDeadline();

        ++m_frameCount;

        const auto now = std::chrono::steady_clock::now();
        m_deltaTime = std::chrono::duration<double>(now - m_lastFrameTime).count();
        m_globalTime = std::chrono::duration<double>(now - m_startTime).count();
        m_lastFrameTime = now;

        if (m_frameCount > 1 and m_deltaTime > 0.0) {
            const double instantFrameRate = 1.0 / m_deltaTime;
            const double smoothing = m_measuredFrameRate > 0.0 ? 0.1 : 1.0;
            m_measuredFrameRate += (instantFrameRate - m_measuredFrameRate) * smoothing;
        }
    }

    void Lifecycle::waitForFrameDeadline()
    {
        if (m_targetFrameRate <= 0.0) {
            m_hasFrameDeadline = false;
            return;
        }

        const auto interval = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<double> {1.0 / m_targetFrameRate}
        );

        if (not m_hasFrameDeadline) {
            m_frameDeadline = std::chrono::steady_clock::now() + interval;
            m_hasFrameDeadline = true;
            return;
        }

        sleepUntil(m_frameDeadline);

        m_frameDeadline += interval;

        if (const auto now = std::chrono::steady_clock::now(); m_frameDeadline < now) {
            m_frameDeadline = now + interval;
        }
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

    void Lifecycle::setFrameRate(double framesPerSecond)
    {
        m_targetFrameRate = framesPerSecond > 0.0 ? framesPerSecond : 0.0;
        m_hasFrameDeadline = false;
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

    double Lifecycle::frameRate() const
    {
        return m_targetFrameRate;
    }

    double Lifecycle::measuredFrameRate() const
    {
        return m_measuredFrameRate;
    }
} // namespace p5
