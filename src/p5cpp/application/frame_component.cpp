#include <p5cpp/application/frame_component.hpp>

#include <p5cpp/system/timing.hpp>

namespace p5cpp
{
    FrameComponent::FrameComponent()
        : m_deltaTime(0.0f),
          m_globalTime(0.0f),
          m_framesPerSecond(0.0f),
          m_frameCount(0),
          m_targetFrameRate(120),
          m_closeRequested(false),
          m_restartRequested(false),
          m_isPaused(false),
          m_exitCode(0),
          m_fpsCalculationInterval(1.0f),
          m_framesPerCalculation(0),
          m_lastCalculationTimestamp(std::chrono::steady_clock::now()),
          m_frameStartTimestamp(std::chrono::steady_clock::now()),
          m_lastFrameStart(std::chrono::steady_clock::now())
    {
    }

    void FrameComponent::update()
    {
        m_frameStartTimestamp = std::chrono::steady_clock::now();
        m_deltaTime = std::chrono::duration<float>(m_frameStartTimestamp - m_lastFrameStart).count();
        m_lastFrameStart = m_frameStartTimestamp;

        m_globalTime += m_deltaTime;

        if (not m_isPaused) {
            ++m_framesPerCalculation;
            ++m_frameCount;

            const auto now = std::chrono::steady_clock::now();
            const float elapsed = std::chrono::duration<float>(now - m_lastCalculationTimestamp).count();

            if (elapsed >= m_fpsCalculationInterval) {
                m_framesPerSecond = static_cast<float>(m_framesPerCalculation) / elapsed;
                m_framesPerCalculation = 0;
                m_lastCalculationTimestamp = now;
            }
        }

        const int desiredFrameRate = m_targetFrameRate;
        if (desiredFrameRate > 0) {
            const auto frameEndTimestamp = std::chrono::steady_clock::now();
            const float frameDuration = std::chrono::duration<float>(frameEndTimestamp - m_frameStartTimestamp).count();
            const float desiredFrameTime = 1.0f / static_cast<float>(desiredFrameRate);
            const float sleepDuration = desiredFrameTime - frameDuration;

            if (sleepDuration > 0.0f) {
                const auto timestampToWaitUntil = frameEndTimestamp + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(sleepDuration));
                precise_sleep_until(timestampToWaitUntil);
            }
        }
    }

    void FrameComponent::loop()
    {
        m_isPaused = false;
    }

    void FrameComponent::noLoop()
    {
        m_isPaused = true;
    }

    void FrameComponent::frameRate(int frameRate)
    {
        m_targetFrameRate = frameRate;
    }

    void FrameComponent::quit()
    {
        m_closeRequested = true;
    }

    void FrameComponent::quit(int exitCode)
    {
        m_exitCode = exitCode;
        m_closeRequested = true;
    }

    void FrameComponent::exitCode(int exitCode)
    {
        m_exitCode = exitCode;
    }

    void FrameComponent::restart()
    {
        m_restartRequested = true;
        m_closeRequested = true;
    }

    bool FrameComponent::isLooping() const
    {
        return not m_isPaused;
    }

    int FrameComponent::getFrameCount() const
    {
        return static_cast<int>(m_frameCount);
    }

    int FrameComponent::getFrameRate() const
    {
        return static_cast<int>(m_framesPerSecond);
    }

    float FrameComponent::getDeltaTime() const
    {
        return m_deltaTime;
    }

    float FrameComponent::getGlobalTime() const
    {
        return m_globalTime;
    }

    bool FrameComponent::isCloseRequested() const
    {
        return m_closeRequested;
    }

    bool FrameComponent::isRestartRequested() const
    {
        return m_restartRequested;
    }
} // namespace p5cpp
