#include <p5cpp/graphics/texture.hpp>
#include <p5cpp/graphics/framebuffer.hpp>
#include <p5cpp/graphics/glyph_atlas_texture_factory.hpp>
#include <p5cpp/graphics/pixel_ops.hpp>
#include <p5cpp/application/logging.hpp>

#include <glad/glad.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <cassert>
#include <cstddef>
#include <vector>

namespace p5cpp::detail
{
    struct TextureResource
    {
        GLuint id;

        explicit TextureResource(GLuint id)
            : id(id)
        {
        }

        ~TextureResource()
        {
            glDeleteTextures(1, &id);
        }
    };
} // namespace p5cpp::detail

namespace p5cpp
{
    Texture loadTexture(uint32_t width, uint32_t height, const color_t* data)
    {
        GLuint textureId = 0;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<const GLchar*>(data));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        return Texture(std::make_shared<detail::TextureResource>(textureId), uint2 {width, height}, TextureFormat::rgba8);
    }

    namespace detail
    {
        Texture GlyphAtlasTextureFactory::make(uint32_t width, uint32_t height)
        {
            GLuint textureId = 0;
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);

            // Zero-initialize: glTexImage2D(..., nullptr) leaves GPU memory undefined
            // (not guaranteed to be 0). With GL_LINEAR filtering, sampling near a packed
            // glyph's UV-rect edge (only 1px padding) can read a texel or two of
            // never-written atlas space — undefined coverage there blends a visible gray
            // halo around glyphs, most noticeable at large text sizes. An all-zero atlas
            // guarantees that bleed reads as "no coverage" instead of garbage.
            const std::vector<uint8_t> zeroed(static_cast<size_t>(width) * static_cast<size_t>(height), 0);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, GL_RED, GL_UNSIGNED_BYTE, zeroed.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            constexpr GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_RED};
            glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

            glBindTexture(GL_TEXTURE_2D, 0);

            return Texture(std::make_shared<TextureResource>(textureId), uint2 {width, height}, TextureFormat::r8);
        }
    } // namespace detail
} // namespace p5cpp

namespace p5cpp
{
    Texture::Texture()
        : m_resource(nullptr),
          m_size {0, 0},
          m_format(TextureFormat::rgba8)
    {
    }

    Texture::Texture(std::shared_ptr<detail::TextureResource> resource, uint2 size, TextureFormat format)
        : m_resource(std::move(resource)),
          m_size(size),
          m_format(format)
    {
    }

    bool Texture::isValid() const
    {
        return m_resource != nullptr;
    }

    TextureId Texture::getTextureId() const
    {
        if (!m_resource) {
            return TextureId {.value = 0};
        }

        return TextureId {.value = m_resource->id};
    }

    uint2 Texture::getSize() const
    {
        return m_size;
    }

    TextureFormat Texture::getFormat() const
    {
        return m_format;
    }

    void Texture::upload(std::span<const color_t> data)
    {
        updateRegion(0, 0, m_size.x, m_size.y, std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data.data()), data.size() * sizeof(color_t)));
    }

    void Texture::updateRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height, std::span<const uint8_t> rawBytes)
    {
        if (!m_resource) {
            return;
        }

        const GLenum glFormat = (m_format == TextureFormat::r8) ? GL_RED : GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, m_resource->id);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(x), static_cast<GLint>(y), static_cast<GLsizei>(width), static_cast<GLsizei>(height), glFormat, GL_UNSIGNED_BYTE, rawBytes.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    Pixels Texture::loadPixels() const
    {
        if (!m_resource || m_format != TextureFormat::rgba8) {
            return Pixels();
        }

        std::vector<color_t> raw(static_cast<size_t>(m_size.x) * static_cast<size_t>(m_size.y));

        glBindTexture(GL_TEXTURE_2D, m_resource->id);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, raw.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        // GL row order is bottom-to-top; Pixels is top-left-origin.
        return Pixels(m_size.x, m_size.y, detail::flipRows(raw, m_size.x, m_size.y));
    }

    void Texture::updatePixels(const Pixels& pixels)
    {
        if (!m_resource || m_format != TextureFormat::rgba8) {
            return;
        }

        assert(pixels.getSize() == m_size);

        // Pixels is top-left-origin; GL row order is bottom-to-top (matches upload()).
        upload(detail::flipRows(std::span<const color_t>(pixels.data(), pixels.size()), m_size.x, m_size.y));
    }
} // namespace p5cpp

namespace p5cpp
{
    Texture loadTexture(const std::filesystem::path& imageFilePath)
    {
        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_set_flip_vertically_on_load(true);

        // Force 4 channels (STBI_rgb_alpha) so the decoded buffer is directly usable as
        // raw R,G,B,A bytes — the same memory layout loadTexture()/Texture::upload()
        // already expect.
        stbi_uc* data = stbi_load(imageFilePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (data == nullptr) {
            error("Failed to load image: " + imageFilePath.string());
            return Texture();
        }

        Texture texture = loadTexture(static_cast<uint32_t>(width), static_cast<uint32_t>(height), reinterpret_cast<const color_t*>(data));
        stbi_image_free(data);

        return texture;
    }

    Texture loadTexture(std::span<const uint8_t> imageData)
    {
        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_set_flip_vertically_on_load(true);

        stbi_uc* data = stbi_load_from_memory(imageData.data(), static_cast<int>(imageData.size()), &width, &height, &channels, STBI_rgb_alpha);
        if (data == nullptr) {
            error("Failed to decode in-memory image data");
            return Texture();
        }

        Texture texture = loadTexture(static_cast<uint32_t>(width), static_cast<uint32_t>(height), reinterpret_cast<const color_t*>(data));
        stbi_image_free(data);

        return texture;
    }

    bool saveTexture(const std::filesystem::path& imageFilePath, const Framebuffer& framebuffer)
    {
        const uint2 size = framebuffer.getSize();

        // readPixels() returns rows bottom-to-top (raw GL order); PNG rows are
        // top-to-bottom, so flip before encoding.
        std::vector<color_t> flipped = detail::flipRows(framebuffer.readPixels(), size.x, size.y);

        return saveTexture(imageFilePath, size.x, size.y, flipped);
    }

    bool saveTexture(const std::filesystem::path& imageFilePath, uint32_t width, uint32_t height, std::span<const color_t> pixels)
    {
        const int result = stbi_write_png(imageFilePath.string().c_str(), static_cast<int>(width), static_cast<int>(height), 4, pixels.data(), static_cast<int>(width) * 4);
        if (result == 0) {
            error("Failed to save image: " + imageFilePath.string());
            return false;
        }

        return true;
    }
} // namespace p5cpp
