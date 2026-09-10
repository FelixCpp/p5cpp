#pragma once

#include <p5cpp/p5cpp.hpp>

namespace p5::webcam
{
    class GpuPixelStream
    {
    public:
        // pixels must use Pixels' usual top-down convention (row 0 = top of the image, like
        // Texture::loadPixels()'s output) -- feed() takes care of getting that onto the GPU in
        // Texture's bottom-up convention correctly.
        void feed(const Pixels& pixels);

        Texture getTexture() const;

    private:
        Texture m_texture;
    };
} // namespace p5::webcam
