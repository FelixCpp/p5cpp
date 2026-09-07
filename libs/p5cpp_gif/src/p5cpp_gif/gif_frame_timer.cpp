#include <p5cpp_gif/gif_frame_timer.hpp>

namespace p5::gif
{
    namespace
    {
        template <typename... T>
        struct visitors : T...
        {
            using T::operator()...;
        };
    } // namespace

    GifFrameTimer::GifFrameTimer(const GifStopCondition& condition, float framesPerSecond)
        : m_frameIntervalInSeconds {1.0f / framesPerSecond},
          m_elapsedTimeSinceStart {0.0f},
          m_elapsedTimeSinceLastCapture {0.0f},
          m_isRecordingComplete {false},
          m_capturedFrameCount {0},
          m_stopCondition {condition}
    {
    }

    void GifFrameTimer::tryCapture(float deltaTimeInSeconds, const std::function<void()>& onCapture)
    {
        if (m_isRecordingComplete) {
            return;
        }

        m_elapsedTimeSinceStart += deltaTimeInSeconds;
        m_elapsedTimeSinceLastCapture += deltaTimeInSeconds;

        if (m_elapsedTimeSinceLastCapture >= m_frameIntervalInSeconds) {
            onCapture();
            m_elapsedTimeSinceLastCapture -= m_frameIntervalInSeconds;
            ++m_capturedFrameCount;
        }

        m_isRecordingComplete = std::visit(
            visitors {
                [this](const RecordForFrameCount& c) {
                    return (m_capturedFrameCount >= c.frameCount);
                },
                [this](const RecordForSeconds& c) {
                    return (m_elapsedTimeSinceStart >= c.seconds);
                },
                [this](const RecordUntil& c) {
                    return c.condition(m_elapsedTimeSinceStart);
                }
            },
            m_stopCondition
        );
    }

    bool GifFrameTimer::isRecordingComplete() const
    {
        return m_isRecordingComplete;
    }

    void GifFrameTimer::cancel()
    {
        m_isRecordingComplete = true;
    }

    std::optional<float> GifFrameTimer::getProgress() const
    {
        return std::visit(
            visitors {
                [this](const RecordForFrameCount& c) -> std::optional<float> {
                    if (c.frameCount == 0) {
                        return 1.0f;
                    }

                    return std::clamp(static_cast<float>(m_capturedFrameCount) / static_cast<float>(c.frameCount), 0.0f, 1.0f);
                },
                [this](const RecordForSeconds& c) -> std::optional<float> {
                    if (c.seconds <= 0.0f) {
                        return 1.0f;
                    }

                    return std::clamp(m_elapsedTimeSinceStart / c.seconds, 0.0f, 1.0f);
                },
                [](const RecordUntil&) -> std::optional<float> {
                    return std::nullopt;
                }
            },
            m_stopCondition
        );
    }
} // namespace p5::gif
