#pragma once

#include <optional>
#include <string>
#include <memory>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <atomic>

#include <p5cpp/p5cpp.hpp>

#include <p5cpp_gif/gif_writer.hpp>
#include <p5cpp_gif/gif_frame_timer.hpp>

namespace p5::gif
{
    struct GifRecordingResource
    {
    public:
        static std::unique_ptr<GifRecordingResource> create(const std::filesystem::path& filepath, const GifStopCondition& condition, const GifRecordingOptions& options, uint32_t width, uint32_t height);

        ~GifRecordingResource();

        void update(float deltaTimeInSeconds, const Graphics& captureSource);
        bool isRecordingComplete() const;
        void cancel();
        std::optional<float> getProgress() const;
        const std::string& getLabel() const;

    private:
        explicit GifRecordingResource(const std::filesystem::path& filepath, std::unique_ptr<GifWriter> gifWriter, std::unique_ptr<PixelReader> pixelReader, const GifStopCondition& condition, float framesPerSecond);

        void requestFrame(const Graphics& captureSource);
        void drainReadyFrames();
        void requestStop();
        void workerLoop();

        std::queue<Pixels> m_queue;
        std::condition_variable m_cv;
        std::atomic<bool> m_stopping;
        std::atomic<bool> m_finished;

        std::string m_label;
        std::unique_ptr<GifWriter> m_gifWriter;
        std::unique_ptr<PixelReader> m_pixelReader;
        int m_outstandingRequests;
        GifFrameTimer m_frameTimer;

        std::mutex m_mutex;
        std::thread m_thread;
    };
} // namespace p5::gif
