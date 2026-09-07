#pragma once

#include <filesystem>

namespace p5
{
    struct Pixels;
}

namespace p5::gif
{
    struct GifWriter
    {
        virtual ~GifWriter() = default;
        virtual bool feed(const Pixels& pixels) = 0;
        virtual bool finish() = 0;
    };
} // namespace p5::gif

namespace p5::gif
{
    std::unique_ptr<GifWriter> createGifFileStreamWriter(const std::filesystem::path& filepath, uint32_t width, uint32_t height, float framesPerSecond);
}
