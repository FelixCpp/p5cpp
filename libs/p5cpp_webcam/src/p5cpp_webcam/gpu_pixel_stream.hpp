#pragma once

#include <p5cpp/p5cpp.hpp>

namespace p5::webcam
{
    class GpuPixelStream
    {
    public:
        void feed(uint32_t width, uint32_t height, std::span<const uint8_t> data);

        Texture getTexture() const;

    private:
        Texture m_texture;
    };
} // namespace p5::webcam
