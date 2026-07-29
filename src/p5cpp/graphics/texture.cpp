#include <p5cpp/graphics/texture.hpp>
#include <p5cpp/graphics/framebuffer.hpp>
#include <p5cpp/application/logging.hpp>

#include <glad/glad.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace p5cpp
{
    Texture loadTexture(uint32_t width, uint32_t height, const color_t* data)
    {
        GLuint glId = 0;
        glGenTextures(1, &glId);
        glBindTexture(GL_TEXTURE_2D, glId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<const GLchar*>(data));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        return Texture {TextureId {glId}, uint2 {width, height}, TextureFormat::rgba8};
    }

    Texture makeGlyphAtlasTexture(int width, int height)
    {
        GLuint glId = 0;
        glGenTextures(1, &glId);
        glBindTexture(GL_TEXTURE_2D, glId);

        // Zero-initialize: glTexImage2D(..., nullptr) leaves GPU memory undefined (not
        // guaranteed to be 0). With GL_LINEAR filtering, sampling near a packed glyph's
        // UV-rect edge (only 1px padding, see GlyphAtlas's paddingX/Y in font.cpp) can
        // read a texel or two of never-written atlas space — undefined coverage there
        // blends a visible gray halo around glyphs, most noticeable at large text sizes.
        // An all-zero atlas guarantees that bleed reads as "no coverage" instead of
        // garbage.
        const std::vector<uint8_t> zeroed(static_cast<size_t>(width) * static_cast<size_t>(height), 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, GL_RED, GL_UNSIGNED_BYTE, zeroed.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        constexpr GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_RED};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
        glBindTexture(GL_TEXTURE_2D, 0);

        return Texture {TextureId {glId}, uint2 {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}, TextureFormat::r8};
    }

    void unload(Texture& texture)
    {
        if (!isTextureValid(texture))
            return;

        const GLuint glId = texture.id.value;
        glDeleteTextures(1, &glId);
        texture = Texture();
    }

    bool isTextureValid(const Texture& texture)
    {
        return texture.id.value != 0;
    }

    void upload(Texture& texture, std::span<const color_t> data)
    {
        const auto* bytes = reinterpret_cast<const uint8_t*>(data.data());
        updateRegion(texture, 0, 0, static_cast<int>(texture.size.x), static_cast<int>(texture.size.y), std::span<const uint8_t>(bytes, data.size() * sizeof(color_t)));
    }

    void updateRegion(Texture& texture, int x, int y, int width, int height, std::span<const uint8_t> data)
    {
        const GLenum glFormat = texture.format == TextureFormat::r8 ? GL_RED : GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, texture.id.value);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, static_cast<GLsizei>(width), static_cast<GLsizei>(height), glFormat, GL_UNSIGNED_BYTE, data.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    Texture loadImage(const std::filesystem::path& imageFilePath)
    {
        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_set_flip_vertically_on_load(true);

        // Force 4 channels (STBI_rgb_alpha) so the decoded buffer is directly usable as
        // raw R,G,B,A bytes — the same memory layout loadTexture()/upload() already
        // expect (see glTexImage2D(..., GL_RGBA, GL_UNSIGNED_BYTE, ...) above).
        stbi_uc* data = stbi_load(imageFilePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (data == nullptr) {
            error("Failed to load image: " + imageFilePath.string());
            return Texture{};
        }

        Texture texture = loadTexture(static_cast<uint32_t>(width), static_cast<uint32_t>(height), reinterpret_cast<const color_t*>(data));
        stbi_image_free(data);

        return texture;
    }

    Texture loadImage(std::span<const uint8_t> imageData)
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

    bool saveImage(const std::filesystem::path& imageFilePath, const Framebuffer& framebuffer)
    {
        const uint2 size = framebuffer.size;
        std::vector<color_t> pixels = readPixels(framebuffer);

        // readPixels() returns rows bottom-to-top (raw GL order); PNG rows are
        // top-to-bottom, so flip before encoding.
        std::vector<color_t> flipped(pixels.size());
        for (uint32_t y = 0; y < size.y; ++y) {
            const size_t srcRow = static_cast<size_t>(size.y - 1 - y) * size.x;
            const size_t dstRow = static_cast<size_t>(y) * size.x;
            std::copy_n(pixels.begin() + static_cast<ptrdiff_t>(srcRow), size.x, flipped.begin() + static_cast<ptrdiff_t>(dstRow));
        }

        return saveImage(imageFilePath, size.x, size.y, flipped);
    }

    bool saveImage(const std::filesystem::path& imageFilePath, uint32_t width, uint32_t height, std::span<const color_t> pixels)
    {
        const int result = stbi_write_png(imageFilePath.string().c_str(), static_cast<int>(width), static_cast<int>(height), 4, pixels.data(), static_cast<int>(width) * 4);
        if (result == 0) {
            error("Failed to save image: " + imageFilePath.string());
            return false;
        }

        return true;
    }
} // namespace p5cpp
