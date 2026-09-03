#include <p5cpp/p5cpp.hpp>
#include <p5cpp_gif/p5cpp_gif.hpp>

#include <msf_gif.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <queue>
#include <thread>

namespace p5::gif
{
    class GIFFileStream
    {
        inline static constexpr int GIF_QUALITY = 16;

    public:
        static std::unique_ptr<GIFFileStream> create(const std::filesystem::path& filepath, uint32_t width, uint32_t height, float framesPerSecond)
        {
            auto stream = std::unique_ptr<GIFFileStream>(new GIFFileStream(filepath, width, height, framesPerSecond));
            if (not stream->m_fileStream) {
                error("Failed to open \"{}\" for writing", filepath.string());
                return nullptr;
            }

            if (not msf_gif_begin_to_file(&stream->m_gifState, static_cast<int>(width), static_cast<int>(height), &GIFFileStream::write, &stream->m_fileStream)) {
                error("Failed to initialize GIF recording for file: {}", filepath.string());
                return nullptr;
            }

            return stream;
        }

        bool feed(const Pixels& pixels)
        {
            if (static_cast<uint32_t>(pixels.width) != m_width or static_cast<uint32_t>(pixels.height) != m_height) {
                error("Recording frame size mismatch: expected {}x{}, got {}x{}", m_width, m_height, pixels.width, pixels.height);
                return false;
            }

            for (uint32_t i = 0; i < pixels.width * pixels.height; ++i) {
                color_t c = pixels.data[i];
                m_bytes[i * 4 + 0] = getRed(c);
                m_bytes[i * 4 + 1] = getGreen(c);
                m_bytes[i * 4 + 2] = getBlue(c);
                m_bytes[i * 4 + 3] = getAlpha(c);
            }

            const int centiSecondsPerFrame = std::max(1, static_cast<int>(std::lround(100.0f / m_framesPerSecond)));
            if (not msf_gif_frame_to_file(&m_gifState, m_bytes.data(), centiSecondsPerFrame, GIF_QUALITY, static_cast<int>(m_width * 4))) {
                warn("Failed to write frame to GIF file");
                return false;
            }

            return true;
        }

        bool finish()
        {
            if (not msf_gif_end_to_file(&m_gifState)) {
                error("Failed to finalize GIF recording");
                return false;
            }

            return true;
        }

    private:
        explicit GIFFileStream(const std::filesystem::path& filepath, uint32_t width, uint32_t height, float framesPerSecond)
            : m_gifState {},
              m_fileStream(filepath, std::ios::binary),
              m_width(width),
              m_height(height),
              m_framesPerSecond(framesPerSecond),
              m_bytes(width * height * 4, 0)
        {
        }

        static size_t write(const void* buffer, size_t size, size_t count, void* stream)
        {
            std::ofstream* fileStream = static_cast<std::ofstream*>(stream);
            const size_t bytesWrittenBefore = fileStream->tellp();
            fileStream->write(static_cast<const char*>(buffer), size * count);
            const size_t bytesWrittenAfter = fileStream->tellp();
            return bytesWrittenAfter - bytesWrittenBefore;
        }

        MsfGifState m_gifState;
        std::ofstream m_fileStream;
        uint32_t m_width;
        uint32_t m_height;
        float m_framesPerSecond;
        std::vector<uint8_t> m_bytes;
    };
} // namespace p5::gif

namespace p5::gif
{
    class GIFFrameTimer
    {
    public:
        GIFFrameTimer(float durationInSeconds, float framesPerSecond)
            : m_durationInSeconds(durationInSeconds),
              m_frameIntervalInSeconds(1.0f / framesPerSecond),
              m_elapsedTimeSinceStart(0.0f),
              m_elapsedTimeSinceLastCapture(0.0f),
              m_isRecordingComplete(false)
        {
        }

        void tryCapture(float deltaTimeInSeconds, std::invocable auto&& callback)
        {
            if (m_isRecordingComplete) {
                return;
            }

            m_elapsedTimeSinceStart += deltaTimeInSeconds;
            m_elapsedTimeSinceLastCapture += deltaTimeInSeconds;

            if (m_elapsedTimeSinceLastCapture >= m_frameIntervalInSeconds) {
                callback();
                m_elapsedTimeSinceLastCapture -= m_frameIntervalInSeconds;
            }

            if (m_elapsedTimeSinceStart >= m_durationInSeconds) {
                m_isRecordingComplete = true;
            }
        }

        bool isRecordingComplete() const
        {
            return m_isRecordingComplete;
        }

    private:
        float m_durationInSeconds;
        float m_frameIntervalInSeconds;
        float m_elapsedTimeSinceStart;
        float m_elapsedTimeSinceLastCapture;
        bool m_isRecordingComplete;
    };
} // namespace p5::gif

namespace p5::gif
{
    class AsyncGIFFrameSink
    {
    public:
        static std::unique_ptr<AsyncGIFFrameSink> create(const std::filesystem::path& filepath, float durationInSeconds, float framesPerSecond, uint32_t width, uint32_t height)
        {
            std::unique_ptr<GIFFileStream> fileStream = GIFFileStream::create(filepath, width, height, framesPerSecond);
            if (fileStream == nullptr) {
                return nullptr;
            }

            std::unique_ptr<PixelReader> pixelReader = createPixelReader(width, height);
            if (pixelReader == nullptr) {
                return nullptr;
            }

            return std::unique_ptr<AsyncGIFFrameSink>(new AsyncGIFFrameSink(std::move(fileStream), std::move(pixelReader), durationInSeconds, framesPerSecond));
        }

        ~AsyncGIFFrameSink()
        {
            requestStop();

            if (m_thread.joinable()) {
                m_thread.join();
            }
        }

        void update(float deltaTimeInSeconds)
        {
            m_frameTimer.tryCapture(deltaTimeInSeconds, [this] {
                requestFrame();
            });

            drainReadyFrames();

            // Only stop once every requested frame has actually been read back and handed to
            // the encoder -- otherwise the last one to a few in-flight PBO readbacks (see
            // requestPixelReadback()/pollPixelReadback()) would still be settling when the
            // encoder thread finishes and finalizes the file, silently truncating the GIF's last
            // frames.
            if (m_frameTimer.isRecordingComplete() and m_outstandingRequests == 0) {
                requestStop();
            }
        }

        bool isRecordingComplete() const
        {
            return m_finished.load(std::memory_order_acquire);
        }

    private:
        explicit AsyncGIFFrameSink(std::unique_ptr<GIFFileStream> fileStream, std::unique_ptr<PixelReader> pixelReader, float durationInSeconds, float framesPerSecond)
            : m_queue {},
              m_cv {},
              m_stopping {false},
              m_finished {false},
              m_fileStream {std::move(fileStream)},
              m_pixelReader {std::move(pixelReader)},
              m_outstandingRequests {0},
              m_frameTimer {durationInSeconds, framesPerSecond},
              m_mutex {},
              m_thread {&AsyncGIFFrameSink::workerLoop, this}
        {
        }

        void requestFrame()
        {
            // flush() first: the graphics target's colorTexture only reflects draw calls the
            // Renderer has actually submitted, not ones still batched -- same precondition
            // loadPixels() has always relied on (see Canvas::loadPixels()).
            flush();

            Graphics graphics = peekGraphics();
            if (not graphics.isValid()) {
                error("GIF recording: requestFrame() called with no graphics pushed");
                return;
            }

            // requestPixelReadback() never blocks; it may drop an older, not-yet-drained readback
            // to make room instead of stalling. In that case a slot that was already counted as
            // outstanding is simply being reused, not added to, so the outstanding count doesn't
            // change.
            if (requestPixelReadback(*m_pixelReader, graphics.colorTexture)) {
                ++m_outstandingRequests;
            }
        }

        void drainReadyFrames()
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

        void requestStop()
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

        void workerLoop()
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

                m_fileStream->feed(pixels);
            }

            m_fileStream->finish();
            m_finished.store(true, std::memory_order_release);
        }

        std::queue<Pixels> m_queue;
        std::condition_variable m_cv;
        std::atomic<bool> m_stopping;
        std::atomic<bool> m_finished;

        std::unique_ptr<GIFFileStream> m_fileStream;
        std::unique_ptr<PixelReader> m_pixelReader;
        int m_outstandingRequests;
        GIFFrameTimer m_frameTimer;

        std::mutex m_mutex;
        std::thread m_thread;
    };
} // namespace p5::gif

namespace p5::gif
{
    class GIFRecorder
    {
    public:
        bool insertRecording(const std::filesystem::path& filepath, float recordingDurationInSeconds, int frameRatePerSecond)
        {
            std::shared_ptr<AsyncGIFFrameSink> recording = AsyncGIFFrameSink::create(filepath, recordingDurationInSeconds, frameRatePerSecond, getWidth(), getHeight());
            if (recording == nullptr) {
                return false;
            }

            m_recordings.push_back(std::move(recording));
            return true;
        }

        void updateRecordings()
        {
            const float deltaTimeInSeconds = static_cast<float>(getDeltaTime());

            for (auto itr = m_recordings.begin(); itr != m_recordings.end();) {
                std::shared_ptr<AsyncGIFFrameSink>& recording = *itr;
                recording->update(deltaTimeInSeconds);

                if (recording->isRecordingComplete()) {
                    itr = m_recordings.erase(itr);
                } else {
                    ++itr;
                }
            }
        }

        void drawRecordingOverlay()
        {
            const bool isRecording = not m_recordings.empty();
            if (not isRecording) {
                return;
            }

            with(
                [] {
                    textSize(24.0f);
                    fill(rgba(255));
                    noStroke();
                    textAlign(TextAlignment::topLeft);
                    text("Recording Gif ...", 40.0f, 10.0f);

                    stroke(rgba(255));
                    strokeWeight(2.0f);
                    fill(rgba(100.0f + (std::sin(getGlobalTime() * 10.0f) * 0.5f + 0.5f) * (255.0f - 100.0f), 0, 0));
                    circle(20.0f, 22.5f, 10.0f);
                },
                false
            );
        }

    private:
        std::vector<std::shared_ptr<AsyncGIFFrameSink>> m_recordings;
    };
} // namespace p5::gif

namespace p5::gif
{
    inline static thread_local std::unique_ptr<GIFRecorder> recorder;
}

namespace p5::gif
{
    bool saveGif(const std::filesystem::path& path, float recordingDurationInSeconds, int frameRatePerSecond)
    {
        if (recorder == nullptr) {
            error("GIFRecorder is not initialized. Please add the GIFRecorderPlugin to your sketch.");
            return false;
        }

        return recorder->insertRecording(path, recordingDurationInSeconds, frameRatePerSecond);
    }
} // namespace p5::gif

namespace p5::gif
{
    class GIFRecorderPlugin : public Plugin
    {
    public:
        void setup(Context& context, const Next& next) override
        {
            recorder = std::make_unique<GIFRecorder>();
            context.provide<GIFRecorder>(recorder.get());

            next();
        }

        void draw([[maybe_unused]] Context& context, const Next& next) override
        {
            next();

            recorder->updateRecordings();
            recorder->drawRecordingOverlay();
        }

        void destroy(Context& context, const Next& next) override
        {
            next();

            context.remove<GIFRecorder>();
            recorder.reset();
        }
    };
} // namespace p5::gif

namespace p5::gif
{
    std::unique_ptr<Plugin> createGIFRecorderPlugin()
    {
        return std::make_unique<GIFRecorderPlugin>();
    }
} // namespace p5::gif
