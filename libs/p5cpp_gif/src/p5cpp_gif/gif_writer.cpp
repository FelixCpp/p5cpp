#include <p5cpp_gif/gif_writer.hpp>

#include <p5cpp/p5cpp.hpp>

#include <msf_gif.h>

#include <fstream>

namespace p5::gif
{
    namespace
    {
        inline static constexpr int GIF_QUALITY = 16;

        static size_t write_buffer_data(const void* buffer, size_t size, size_t count, void* stream)
        {
            std::ofstream* fileStream = static_cast<std::ofstream*>(stream);
            const size_t bytesWrittenBefore = fileStream->tellp();
            fileStream->write(static_cast<const char*>(buffer), size * count);
            if (fileStream->fail()) {
                error("Failed to write GIF data to file stream");
                return 0;
            }

            const size_t bytesWrittenAfter = fileStream->tellp();
            return bytesWrittenAfter - bytesWrittenBefore;
        }
    } // namespace

    class GifFileStreamWriter : public GifWriter
    {
    public:
        static std::unique_ptr<GifFileStreamWriter> create(const std::filesystem::path& filepath, uint32_t width, uint32_t height, float framesPerSecond)
        {
            if (framesPerSecond <= 0.0f) {
                error("Could not create GifWriter: framesPerSecond must be greater than 0, got {}", framesPerSecond);
                return nullptr;
            }

            auto stream = std::unique_ptr<GifFileStreamWriter>(new GifFileStreamWriter(filepath, width, height, framesPerSecond));
            if (not stream->m_fileStream) {
                error("Failed to open \"{}\" for writing", filepath.string());
                return nullptr;
            }

            if (not msf_gif_begin_to_file(&stream->m_gifState, static_cast<int>(width), static_cast<int>(height), &write_buffer_data, &stream->m_fileStream)) {
                error("Failed to initialize GIF recording for file: {}", filepath.string());
                return nullptr;
            }

            return stream;
        }

        bool feed(const Pixels& pixels) override
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

        bool finish() override
        {
            if (not msf_gif_end_to_file(&m_gifState)) {
                error("Failed to finalize GIF recording");
                return false;
            }

            return true;
        }

    private:
        GifFileStreamWriter(const std::filesystem::path& filepath, uint32_t width, uint32_t height, float framesPerSecond)
            : m_gifState {},
              m_fileStream(filepath, std::ios::binary),
              m_width(width),
              m_height(height),
              m_framesPerSecond(framesPerSecond),
              m_bytes(width * height * 4, 0)
        {
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
    std::unique_ptr<GifWriter> createGifFileStreamWriter(const std::filesystem::path& filepath, uint32_t width, uint32_t height, float framesPerSecond)
    {
        return GifFileStreamWriter::create(filepath, width, height, framesPerSecond);
    }
} // namespace p5::gif
