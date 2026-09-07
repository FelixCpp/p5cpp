#include <p5cpp_gif/gif_recording_resource.hpp>

namespace p5::gif
{
    std::unique_ptr<GifRecordingResource> GifRecordingResource::create(const std::filesystem::path& filepath, const GifStopCondition& condition, const GifRecordingOptions& options, uint32_t width, uint32_t height)
    {
        std::unique_ptr<GifWriter> fileStream = createGifFileStreamWriter(filepath, width, height, options.framesPerSecond);
        if (fileStream == nullptr) {
            return nullptr;
        }

        std::unique_ptr<PixelReader> pixelReader = createPixelReader(width, height);
        if (pixelReader == nullptr) {
            return nullptr;
        }

        return std::unique_ptr<GifRecordingResource>(new GifRecordingResource(filepath, std::move(fileStream), std::move(pixelReader), condition, options.framesPerSecond));
    }

    GifRecordingResource::~GifRecordingResource()
    {
        requestStop();

        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    void GifRecordingResource::update(float deltaTimeInSeconds, const Graphics& captureSource)
    {
        m_frameTimer.tryCapture(deltaTimeInSeconds, [this, &captureSource] {
            requestFrame(captureSource);
        });

        drainReadyFrames();

        if (m_frameTimer.isRecordingComplete() and m_outstandingRequests == 0) {
            requestStop();
        }
    }

    bool GifRecordingResource::isRecordingComplete() const
    {
        return m_finished.load(std::memory_order_acquire);
    }

    void GifRecordingResource::cancel()
    {
        m_frameTimer.cancel();
    }

    std::optional<float> GifRecordingResource::getProgress() const
    {
        return m_frameTimer.getProgress();
    }

    const std::string& GifRecordingResource::getLabel() const
    {
        return m_label;
    }

    GifRecordingResource::GifRecordingResource(const std::filesystem::path& filepath, std::unique_ptr<GifWriter> gifWriter, std::unique_ptr<PixelReader> pixelReader, const GifStopCondition& condition, float framesPerSecond)
        : m_queue {},
          m_cv {},
          m_stopping {false},
          m_finished {false},
          m_label {filepath.filename().string()},
          m_gifWriter {std::move(gifWriter)},
          m_pixelReader {std::move(pixelReader)},
          m_outstandingRequests {0},
          m_frameTimer {condition, framesPerSecond},
          m_mutex {},
          m_thread {&GifRecordingResource::workerLoop, this}
    {
    }

    void GifRecordingResource::requestFrame(const Graphics& captureSource)
    {
        if (not captureSource.isValid()) {
            error("GIF recording: no capture snapshot available");
            return;
        }

        if (requestPixelReadback(*m_pixelReader, captureSource.colorTexture)) {
            ++m_outstandingRequests;
        }
    }

    void GifRecordingResource::drainReadyFrames()
    {
        while (std::optional<Pixels> pixels = pollPixelReadback(*m_pixelReader)) {
            --m_outstandingRequests;
            {
                std::lock_guard lock(m_mutex);
                m_queue.push(std::move(*pixels));
            }
            m_cv.notify_one();
        }
    }

    void GifRecordingResource::requestStop()
    {
        {
            std::lock_guard lock(m_mutex);
            if (m_stopping) {
                return;
            }
            m_stopping = true;
        }
        m_cv.notify_one();
    }

    void GifRecordingResource::workerLoop()
    {
        while (true) {
            Pixels pixels;

            {
                std::unique_lock lock(m_mutex);
                m_cv.wait(lock, [this] {
                    return not m_queue.empty() or m_stopping;
                });

                if (m_queue.empty() and m_stopping) {
                    break;
                }

                pixels = std::move(m_queue.front());
                m_queue.pop();
            }

            m_gifWriter->feed(pixels);
        }

        m_gifWriter->finish();
        m_finished.store(true, std::memory_order_release);
    }
} // namespace p5::gif
