#include <p5cpp_webcam/gpu_pixel_stream.hpp>

namespace p5::webcam
{
    void GpuPixelStream::feed(const Pixels& pixels)
    {
        const bool hasSizeChanged = m_texture.size.x != pixels.width or m_texture.size.y != pixels.height;
        if (not m_texture.isValid() or hasSizeChanged) {
            std::optional<Texture> recreated = loadTexture(pixels.width, pixels.height, {}, TexturePixelFormat::rgba8);

            if (not recreated.has_value()) {
                error("GpuPixelStream: failed to (re)create the frame texture");
                return;
            }

            m_texture = std::move(recreated).value();
        }

        // updatePixels() flips pixels' top-down rows into the texture's bottom-up GL storage --
        // skipping it and uploading pixels.data as-is (as this used to do via updateSubImage())
        // renders the frame vertically mirrored.
        m_texture.updatePixels(pixels);
    }

    Texture GpuPixelStream::getTexture() const
    {
        return m_texture;
    }
} // namespace p5::webcam
