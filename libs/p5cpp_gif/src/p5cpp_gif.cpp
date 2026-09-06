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

namespace
{
    template <typename... T>
    struct visitors : T...
    {
        using T::operator()...;
    };

} // namespace

namespace p5::gif
{
    class GIFFileStream
    {
        inline static constexpr int GIF_QUALITY = 16;

    public:
        static std::unique_ptr<GIFFileStream> create(const std::filesystem::path& filepath, uint32_t width, uint32_t height, float framesPerSecond)
        {
            if (framesPerSecond <= 0.0f) {
                error("Could not create GIFFileStream: framesPerSecond must be greater than 0, got {}", framesPerSecond);
                return nullptr;
            }

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
        GIFFrameTimer(const GifStopCondition& condition, float framesPerSecond)
            : m_frameIntervalInSeconds {1.0f / framesPerSecond},
              m_elapsedTimeSinceStart {0.0f},
              m_elapsedTimeSinceLastCapture {0.0f},
              m_isRecordingComplete {false},
              m_capturedFrameCount {0},
              m_stopCondition {condition}
        {
        }

        void tryCapture(float deltaTimeInSeconds, std::invocable auto&& onCapture)
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
                    [this](const RecordForFrameCount& c) { return (m_capturedFrameCount >= c.frameCount); },
                    [this](const RecordForSeconds& c) { return (m_elapsedTimeSinceStart >= c.seconds); },
                    [this](const RecordUntil& c) { return c.condition(m_elapsedTimeSinceStart); }
                },
                m_stopCondition
            );
        }

        bool isRecordingComplete() const
        {
            return m_isRecordingComplete;
        }

        void cancel()
        {
            m_isRecordingComplete = true;
        }

        std::optional<float> getProgress() const
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
                    [](const RecordUntil&) -> std::optional<float> { return std::nullopt; }
                },
                m_stopCondition
            );
        }

    private:
        float m_frameIntervalInSeconds;
        float m_elapsedTimeSinceStart;
        float m_elapsedTimeSinceLastCapture;
        bool m_isRecordingComplete;

        size_t m_capturedFrameCount;

        GifStopCondition m_stopCondition;
    };
} // namespace p5::gif

namespace p5::gif
{
    struct GifRecordingResource
    {
    public:
        static std::unique_ptr<GifRecordingResource> create(const std::filesystem::path& filepath, const GifStopCondition& condition, const GifRecordingOptions& options, uint32_t width, uint32_t height)
        {
            std::unique_ptr<GIFFileStream> fileStream = GIFFileStream::create(filepath, width, height, options.framesPerSecond);
            if (fileStream == nullptr) {
                return nullptr;
            }

            std::unique_ptr<PixelReader> pixelReader = createPixelReader(width, height);
            if (pixelReader == nullptr) {
                return nullptr;
            }

            return std::unique_ptr<GifRecordingResource>(new GifRecordingResource(filepath, std::move(fileStream), std::move(pixelReader), condition, options.framesPerSecond));
        }

        ~GifRecordingResource()
        {
            requestStop();

            if (m_thread.joinable()) {
                m_thread.join();
            }
        }

        // captureSource is a snapshot of the canvas taken *before* the recording overlay was drawn
        // this frame (see GIFRecorder::refreshCaptureSnapshot()) -- requestFrame() never reads the
        // live/on-screen canvas directly, so nothing this plugin (or any other) draws on top of it
        // afterwards can ever end up in the recorded file.
        void update(float deltaTimeInSeconds, const Graphics& captureSource)
        {
            m_frameTimer.tryCapture(deltaTimeInSeconds, [this, &captureSource] {
                requestFrame(captureSource);
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

        void cancel()
        {
            m_frameTimer.cancel();
        }

        std::optional<float> getProgress() const
        {
            return m_frameTimer.getProgress();
        }

        const std::string& getLabel() const
        {
            return m_label;
        }

    private:
        explicit GifRecordingResource(const std::filesystem::path& filepath, std::unique_ptr<GIFFileStream> fileStream, std::unique_ptr<PixelReader> pixelReader, const GifStopCondition& condition, float framesPerSecond)
            : m_queue {},
              m_cv {},
              m_stopping {false},
              m_finished {false},
              m_label {filepath.filename().string()},
              m_fileStream {std::move(fileStream)},
              m_pixelReader {std::move(pixelReader)},
              m_outstandingRequests {0},
              m_frameTimer {condition, framesPerSecond},
              m_mutex {},
              m_thread {&GifRecordingResource::workerLoop, this}
        {
        }

        void requestFrame(const Graphics& captureSource)
        {
            if (not captureSource.isValid()) {
                error("GIF recording: no capture snapshot available");
                return;
            }

            // requestPixelReadback() never blocks; it may drop an older, not-yet-drained readback
            // to make room instead of stalling. In that case a slot that was already counted as
            // outstanding is simply being reused, not added to, so the outstanding count doesn't
            // change.
            if (requestPixelReadback(*m_pixelReader, captureSource.colorTexture)) {
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

        std::string m_label;
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
    // Single source of truth for the overlay renderer -- defaults to the built-in badge; replaced
    // wholesale (or cleared) via setGifRecordingOverlayCallback(). Declared ahead of GIFRecorder
    // since GIFRecorder::drawRecordingOverlay() reads it directly.
    inline static thread_local GifRecordingOverlayCallback overlayCallback = &defaultGifRecordingOverlay;
} // namespace p5::gif

namespace p5::gif
{
    class GIFRecorder
    {
    public:
        std::shared_ptr<GifRecordingResource> insertRecording(const std::filesystem::path& filepath, const GifStopCondition& condition, const GifRecordingOptions& options)
        {
            if (options.framesPerSecond <= 0.0f) {
                error("recordGif(): framesPerSecond must be > 0, got {}", options.framesPerSecond);
                return nullptr;
            }

            const bool isConditionValid = std::visit(
                visitors {
                    [](const RecordForFrameCount& c) { return c.frameCount > 0; },
                    [](const RecordForSeconds& c) { return c.seconds > 0.0f; },
                    [](const RecordUntil& c) { return static_cast<bool>(c.condition); },
                },
                condition
            );
            if (not isConditionValid) {
                error("recordGif(): stop condition is invalid (zero frame count/duration, or an empty predicate)");
                return nullptr;
            }

            std::shared_ptr<GifRecordingResource> recording = GifRecordingResource::create(filepath, condition, options, getWidth(), getHeight());
            if (recording == nullptr) {
                return nullptr;
            }

            m_recordings.push_back(recording);
            return recording;
        }

        void updateRecordings()
        {
            refreshCaptureSnapshot();

            const float deltaTimeInSeconds = static_cast<float>(getDeltaTime());

            for (auto itr = m_recordings.begin(); itr != m_recordings.end();) {
                std::shared_ptr<GifRecordingResource>& recording = *itr;
                recording->update(deltaTimeInSeconds, m_captureGraphics);

                if (recording->isRecordingComplete()) {
                    itr = m_recordings.erase(itr);
                } else {
                    ++itr;
                }
            }
        }

        void drawRecordingOverlay()
        {
            if (not overlayCallback or m_recordings.empty()) {
                return;
            }

            std::vector<GifRecordingStatus> status;
            status.reserve(m_recordings.size());

            for (const std::shared_ptr<GifRecordingResource>& recording : m_recordings) {
                status.push_back({
                    .label = recording->getLabel(),
                    .progress = recording->getProgress(),
                });
            }

            overlayCallback(status);
        }

    private:
        void refreshCaptureSnapshot()
        {
            if (m_recordings.empty()) {
                return;
            }

            flush();

            const Graphics live = peekGraphics();
            if (not live.isValid()) {
                error("GIF recording: refreshCaptureSnapshot() called with no graphics pushed");
                return;
            }

            if (not m_captureGraphics.isValid() or m_captureGraphics.size != live.size) {
                std::optional<Graphics> recreated = createGraphics(live.size.x, live.size.y);
                if (not recreated.has_value()) {
                    error("GIF recording: failed to (re)create the capture snapshot target");
                    return;
                }
                m_captureGraphics = std::move(recreated).value();
            }

            withGraphics(
                m_captureGraphics,
                [&live] {
                    image(live, 0.0f, 0.0f, static_cast<float>(live.size.x), static_cast<float>(live.size.y));
                },
                false
            );
        }

        Graphics m_captureGraphics;
        std::vector<std::shared_ptr<GifRecordingResource>> m_recordings;
    };
} // namespace p5::gif

namespace p5::gif
{
    inline static thread_local std::unique_ptr<GIFRecorder> recorder;
} // namespace p5::gif

namespace p5::gif
{
    bool GifRecording::isValid() const
    {
        return resource != nullptr;
    }

    bool GifRecording::isActive() const
    {
        return resource != nullptr and not resource->isRecordingComplete();
    }

    std::optional<float> GifRecording::getProgress() const
    {
        return resource != nullptr ? resource->getProgress() : std::nullopt;
    }

    void GifRecording::cancel()
    {
        if (resource != nullptr) {
            resource->cancel();
        }
    }

    std::optional<GifRecording> recordGif(const std::filesystem::path& path, const GifStopCondition& condition, const GifRecordingOptions& options)
    {
        if (recorder == nullptr) {
            error("GIFRecorder is not initialized. Please add the GIFRecorderPlugin to your sketch.");
            return std::nullopt;
        }

        std::shared_ptr<GifRecordingResource> resource = recorder->insertRecording(path, condition, options);
        if (resource == nullptr) {
            return std::nullopt;
        }

        return GifRecording {.resource = std::move(resource)};
    }

    void setGifRecordingOverlayCallback(GifRecordingOverlayCallback callback)
    {
        overlayCallback = std::move(callback);
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

namespace p5::gif
{
    void defaultGifRecordingOverlay(std::span<const GifRecordingStatus> recordings)
    {
        if (recordings.empty()) {
            return;
        }

        static constexpr float margin = 16.0f;
        static constexpr float badgeHeight = 28.0f;
        static constexpr float badgeSpacing = 8.0f;
        static constexpr float horizontalPadding = 14.0f;

        textAlign(TextAlignment::centerLeft);
        textSize(13.0f);
        noStroke();

        const uint2 canvasSize = getGraphicsSize();
        const float pulse = 0.5f + 0.5f * std::sin(getGlobalTime() * 6.0f);

        float y = margin;
        for (const GifRecordingStatus& recording : recordings) {
            const std::string& label = recording.label;

            const float dotDiameter = badgeHeight * 0.4f;
            const float badgeWidth = horizontalPadding + dotDiameter + 8.0f + textWidth(label) + horizontalPadding;
            const float x = static_cast<float>(canvasSize.x) - margin - badgeWidth;

            // Backdrop pill so the badge stays legible over any sketch content.
            fill(rgba(20, 20, 20, 190));
            rect(x, y, badgeWidth, badgeHeight, BorderRadius::all(badgeHeight * 0.5f));

            // Pulsing "recording" dot.
            fill(rgba(255, static_cast<int32_t>(lerp(70.0f, 110.0f, pulse)), static_cast<int32_t>(lerp(70.0f, 110.0f, pulse))));
            circle(x + horizontalPadding + dotDiameter * 0.5f, y + badgeHeight * 0.5f, dotDiameter);

            // Filename label.
            fill(rgba(255));
            text(label, x + horizontalPadding + dotDiameter + 8.0f, y + badgeHeight * 0.5f);

            // Thin progress bar along the bottom edge of the pill.
            const float barLeft = x + horizontalPadding * 0.5f;
            const float barWidth = badgeWidth - horizontalPadding;
            fill(rgba(255, 255, 255, 60));
            rect(barLeft, y + badgeHeight - 4.0f, barWidth, 2.0f);
            fill(rgba(255, 90, 90));

            if (recording.progress.has_value()) {
                rect(barLeft, y + badgeHeight - 4.0f, barWidth * *recording.progress, 2.0f);
            } else {
                // RecordUntil has no known endpoint -- a sliding marquee segment instead of a fake
                // percentage, so it's honest about "running, but no ETA" rather than lying.
                const float segmentWidth = barWidth * 0.25f;
                const float t = 0.5f + 0.5f * std::sin(getGlobalTime() * 3.0f);
                rect(barLeft + (barWidth - segmentWidth) * t, y + badgeHeight - 4.0f, segmentWidth, 2.0f);
            }

            y += badgeHeight + badgeSpacing;
        }
    }
} // namespace p5::gif
