#include <p5cpp_gif/p5cpp_gif.hpp>

#include <msf_gif.h>

#include <algorithm>
#include <cmath>
#include <fstream>

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

        bool feed(Pixels pixels)
        {
            if (pixels.width != m_width or pixels.height != m_height) {
                error("Recording frame size mismatch: expected {}x{}, got {}x{}", m_width, m_height, pixels.width, pixels.height);
                return false;
            }

            std::vector<uint8_t> bytes(pixels.data.size() * 4);
            for (size_t i = 0; i < pixels.data.size(); ++i) {
                bytes[i * 4 + 0] = getRed(pixels.data[i]);
                bytes[i * 4 + 1] = getGreen(pixels.data[i]);
                bytes[i * 4 + 2] = getBlue(pixels.data[i]);
                bytes[i * 4 + 3] = getAlpha(pixels.data[i]);
            }

            const int centiSecondsPerFrame = std::max(1, static_cast<int>(std::lround(100.0f / m_framesPerSecond)));
            const int stride = static_cast<int>(pixels.width * 4);
            if (not msf_gif_frame_to_file(&m_gifState, bytes.data(), centiSecondsPerFrame, GIF_QUALITY, stride)) {
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
            : m_fileStream(filepath, std::ios::binary),
              m_width(width),
              m_height(height),
              m_framesPerSecond(framesPerSecond)
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

        MsfGifState m_gifState {};
        std::ofstream m_fileStream;
        uint32_t m_width;
        uint32_t m_height;

        float m_framesPerSecond;
    };
} // namespace p5::gif

namespace p5::gif
{
    class GIFRecording
    {
    public:
        static std::unique_ptr<GIFRecording> create(const std::filesystem::path& filepath, const float durationInSeconds, const float framesPerSecond, const uint32_t width, const uint32_t height)
        {
            std::unique_ptr<GIFFileStream> fileStream = GIFFileStream::create(filepath, width, height, framesPerSecond);
            if (fileStream == nullptr) {
                return nullptr;
            }

            return std::unique_ptr<GIFRecording>(new GIFRecording(std::move(fileStream), durationInSeconds, framesPerSecond));
        }

        void update(float deltaTimeInSeconds)
        {
            if (m_isRecordingComplete) {
                return;
            }

            m_elapsedTimeSinceStart += deltaTimeInSeconds;
            m_elapsedTimeSinceLastCapture += deltaTimeInSeconds;

            while (m_elapsedTimeSinceLastCapture >= m_frameIntervalInSeconds) {
                captureFrame(loadPixels());
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

        void finish()
        {
            m_fileStream->finish();
        }

    private:
        void captureFrame(Pixels pixels)
        {
            m_fileStream->feed(std::move(pixels));
        }

        explicit GIFRecording(std::unique_ptr<GIFFileStream> fileStream, float durationInSeconds, float framesPerSecond)
            : m_durationInSeconds {durationInSeconds},
              m_frameIntervalInSeconds {1.0f / framesPerSecond},
              m_elapsedTimeSinceStart {0.0f},
              m_elapsedTimeSinceLastCapture {0.0f},
              m_isRecordingComplete {false},
              m_fileStream {std::move(fileStream)}
        {
        }

        float m_durationInSeconds;
        float m_frameIntervalInSeconds;
        float m_elapsedTimeSinceStart;
        float m_elapsedTimeSinceLastCapture;
        bool m_isRecordingComplete;

        std::unique_ptr<GIFFileStream> m_fileStream;
    };
} // namespace p5::gif

namespace p5::gif
{
    class GIFRecorder
    {
    public:
        bool insertRecording(const std::filesystem::path& filepath, float recordingDurationInSeconds, int frameRatePerSecond)
        {
            std::shared_ptr<GIFRecording> recording = GIFRecording::create(filepath, recordingDurationInSeconds, frameRatePerSecond, getWidth(), getHeight());
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
                std::shared_ptr<GIFRecording>& recording = *itr;
                recording->update(deltaTimeInSeconds);

                if (recording->isRecordingComplete()) {
                    recording->finish();
                    itr = m_recordings.erase(itr);
                } else {
                    ++itr;
                }
            }
        }

    private:
        std::vector<std::shared_ptr<GIFRecording>> m_recordings;
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
