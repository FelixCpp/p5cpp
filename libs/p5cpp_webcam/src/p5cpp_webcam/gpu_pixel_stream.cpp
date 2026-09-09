#include <p5cpp_webcam/gpu_pixel_stream.hpp>

namespace p5::webcam
{
    void GpuPixelStream::feed(uint32_t width, uint32_t height, std::span<const uint8_t> data)
    {
        const bool hasSizeChanged = m_texture.size.x != width or m_texture.size.y != height;
        if (not m_texture.isValid() or hasSizeChanged) {
            std::optional<Texture> recreated = loadTexture(width, height, data, TexturePixelFormat::rgba8);

            if (not recreated.has_value()) {
                error("GpuPixelStream: failed to (re)create the frame texture");
                return;
            }

            m_texture = std::move(recreated).value();
        } else {
            m_texture.updateSubImage(0, 0, width, height, data);
        }
    }

    Texture GpuPixelStream::getTexture() const
    {
        return m_texture;
    }
} // namespace p5::webcam
