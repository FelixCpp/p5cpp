#pragma once

#include <functional>

#include <p5cpp_gif/p5cpp_gif.hpp>

namespace p5::gif
{
    class GifFrameTimer
    {
    public:
        GifFrameTimer(const GifStopCondition& condition, float framesPerSecond);

        void tryCapture(float deltaTimeInSeconds, const std::function<void()>& onCapture);

        bool isRecordingComplete() const;

        void cancel();
        std::optional<float> getProgress() const;

    private:
        float m_frameIntervalInSeconds;
        float m_elapsedTimeSinceStart;
        float m_elapsedTimeSinceLastCapture;
        bool m_isRecordingComplete;

        size_t m_capturedFrameCount;

        GifStopCondition m_stopCondition;
    };
} // namespace p5::gif
