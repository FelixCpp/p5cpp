#include <p5cpp/p5cpp.hpp>

#include <glad/glad.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <algorithm>
#include <optional>

namespace p5
{
    namespace
    {
        struct GLPixelFormat
        {
            GLint internalFormat;
            GLenum externalFormat;
            size_t bytesPerPixel;
        };

        std::optional<GLPixelFormat> toGLPixelFormat(TexturePixelFormat format)
        {
            switch (format) {
                case TexturePixelFormat::rgba8: return GLPixelFormat {GL_RGBA8, GL_RGBA, 4};
                case TexturePixelFormat::r8: return GLPixelFormat {GL_R8, GL_RED, 1};
                default:
                    error("Texture: unknown TexturePixelFormat");
                    return std::nullopt;
            }
        }

        // glGetTexImage() (queryPixelData()) returns rows in OpenGL's bottom-up memory order (row 0 =
        // the texture's bottom edge) -- the same convention loadTextureFromFile() corrects for on the
        // way in via stbi_set_flip_vertically_on_load(). Every other y coordinate in this library
        // increases downward (mouse/draw coordinates, textAlign, ...) and every image file format
        // stores row 0 as the top of the image, so both Pixels and saved files need that same top-down
        // convention -- flip queryPixelData()'s raw bytes at each of those boundaries to match it.
        void flipRowsVertically(std::vector<uint8_t>& pixelData, uint32_t width, uint32_t height)
        {
            const size_t rowBytes = static_cast<size_t>(width) * 4;
            for (uint32_t y = 0; y < height / 2; ++y) {
                uint8_t* top = pixelData.data() + static_cast<size_t>(y) * rowBytes;
                uint8_t* bottom = pixelData.data() + static_cast<size_t>(height - 1 - y) * rowBytes;
                std::swap_ranges(top, top + rowBytes, bottom);
            }
        }
    } // namespace

    Texture::~Texture()
    {
        glDeleteTextures(1, &id);
    }

    std::unique_ptr<Texture> loadTextureFromMemory(uint32_t width, uint32_t height, std::span<const uint8_t> data, TexturePixelFormat format)
    {
        const std::optional<GLPixelFormat> glFormatOpt = toGLPixelFormat(format);
        if (not glFormatOpt.has_value()) {
            return nullptr;
        }
        const GLPixelFormat& glFormat = *glFormatOpt;

        const size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * glFormat.bytesPerPixel;
        if (not data.empty() and data.size() != expectedSize) {
            error("loadTextureFromMemory() data size does not match width * height * bytesPerPixel");
            return nullptr;
        }

        GLuint textureId;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, glFormat.internalFormat, width, height, 0, glFormat.externalFormat, GL_UNSIGNED_BYTE, data.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        auto texture = std::make_unique<Texture>();
        texture->id = textureId;
        texture->size = uint2 {.x = width, .y = height};
        texture->pixelFormat = format;
        return texture;
    }

    std::unique_ptr<Texture> loadTextureFromFile(const std::filesystem::path& filepath)
    {
        typedef decltype(&stbi_image_free) stbi_deleter;

        stbi_set_flip_vertically_on_load(1);

        const std::string filepathStr = filepath.string();
        int width, height, channels;
        std::unique_ptr<stbi_uc, stbi_deleter> pixelData(stbi_load(filepathStr.c_str(), &width, &height, &channels, STBI_rgb_alpha), &stbi_image_free);

        if (pixelData == nullptr) {
            return nullptr;
        }

        return loadTextureFromMemory(static_cast<uint32_t>(width), static_cast<uint32_t>(height), std::span<const uint8_t>(pixelData.get(), width * height * 4));
    }

    void updateSubImage(Texture& texture, uint32_t x, uint32_t y, uint32_t width, uint32_t height, std::span<const uint8_t> data)
    {
        const std::optional<GLPixelFormat> glFormatOpt = toGLPixelFormat(texture.pixelFormat);
        if (not glFormatOpt.has_value()) {
            return;
        }

        const GLPixelFormat& glFormat = glFormatOpt.value();

        const size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * glFormat.bytesPerPixel;
        if (data.size() != expectedSize) {
            error("updateSubImage() data size does not match width * height * bytesPerPixel");
            return;
        }
        if (x + width > texture.size.x or y + height > texture.size.y) {
            error("updateSubImage() region is out of bounds");
            return;
        }

        glBindTexture(GL_TEXTURE_2D, texture.id);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(x), static_cast<GLint>(y), static_cast<GLsizei>(width), static_cast<GLsizei>(height), glFormat.externalFormat, GL_UNSIGNED_BYTE, data.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    std::vector<uint8_t> queryPixelData(const Texture& texture)
    {
        std::vector<uint8_t> pixelData(static_cast<size_t>(texture.size.x) * static_cast<size_t>(texture.size.y) * 4);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData.data());
        glBindTexture(GL_TEXTURE_2D, 0);
        return pixelData;
    }

    bool saveTextureToFileAsPNG(const Texture& texture, const std::filesystem::path& filepath)
    {
        const std::string filepathStr = filepath.string();
        const auto [width, height] = texture.size;
        auto pixelData = queryPixelData(texture);
        flipRowsVertically(pixelData, width, height);
        const int result = stbi_write_png(filepathStr.c_str(), static_cast<int>(width), static_cast<int>(height), STBI_rgb_alpha, pixelData.data(), static_cast<int>(width) * 4);
        return result != 0;
    }

    bool saveTextureToFileAsJPEG(const Texture& texture, const std::filesystem::path& filepath, int quality)
    {
        const std::string filepathStr = filepath.string();
        const auto [width, height] = texture.size;
        auto pixelData = queryPixelData(texture);
        flipRowsVertically(pixelData, width, height);
        const int result = stbi_write_jpg(filepathStr.c_str(), static_cast<int>(width), static_cast<int>(height), STBI_rgb_alpha, pixelData.data(), quality);
        return result != 0;
    }

    bool saveTextureToFileAsBMP(const Texture& texture, const std::filesystem::path& filepath)
    {
        const std::string filepathStr = filepath.string();
        const auto [width, height] = texture.size;
        auto pixelData = queryPixelData(texture);
        flipRowsVertically(pixelData, width, height);
        const int result = stbi_write_bmp(filepathStr.c_str(), static_cast<int>(width), static_cast<int>(height), STBI_rgb_alpha, pixelData.data());
        return result != 0;
    }

    color_t getPixel(const Pixels& pixels, int32_t x, int32_t y)
    {
        if (x < 0 or y < 0 or static_cast<uint32_t>(x) >= pixels.width or static_cast<uint32_t>(y) >= pixels.height) {
            error("getPixel() coordinates ({}, {}) are out of bounds for {}x{} Pixels", x, y, pixels.width, pixels.height);
            return rgba(0, 0);
        }

        return pixels.data[static_cast<size_t>(y) * pixels.width + static_cast<size_t>(x)];
    }

    void setPixel(Pixels& pixels, int32_t x, int32_t y, color_t color)
    {
        if (x < 0 or y < 0 or static_cast<uint32_t>(x) >= pixels.width or static_cast<uint32_t>(y) >= pixels.height) {
            error("setPixel() coordinates ({}, {}) are out of bounds for {}x{} Pixels", x, y, pixels.width, pixels.height);
            return;
        }

        pixels.data[static_cast<size_t>(y) * pixels.width + static_cast<size_t>(x)] = color;
    }

    Pixels loadPixels(const Texture& texture)
    {
        if (texture.pixelFormat != TexturePixelFormat::rgba8) {
            error("loadPixels() only supports TexturePixelFormat::rgba8 textures");
            return {};
        }

        const auto [width, height] = texture.size;
        auto bytes = queryPixelData(texture);
        flipRowsVertically(bytes, width, height);

        Pixels pixels {.width = width, .height = height, .data = std::vector<color_t>(static_cast<size_t>(width) * height)};
        for (size_t i = 0; i < pixels.data.size(); ++i) {
            pixels.data[i] = rgba(bytes[i * 4 + 0], bytes[i * 4 + 1], bytes[i * 4 + 2], bytes[i * 4 + 3]);
        }

        return pixels;
    }

    void updatePixels(Texture& texture, const Pixels& pixels)
    {
        if (texture.pixelFormat != TexturePixelFormat::rgba8) {
            error("updatePixels() only supports TexturePixelFormat::rgba8 textures");
            return;
        }

        const auto [width, height] = texture.size;
        if (pixels.width != width or pixels.height != height) {
            error("updatePixels() Pixels size ({}x{}) does not match texture size ({}x{})", pixels.width, pixels.height, width, height);
            return;
        }

        std::vector<uint8_t> bytes(pixels.data.size() * 4);
        for (size_t i = 0; i < pixels.data.size(); ++i) {
            bytes[i * 4 + 0] = getRed(pixels.data[i]);
            bytes[i * 4 + 1] = getGreen(pixels.data[i]);
            bytes[i * 4 + 2] = getBlue(pixels.data[i]);
            bytes[i * 4 + 3] = getAlpha(pixels.data[i]);
        }

        // Pixels is top-down (see loadPixels()'s flip above); glTexSubImage2D expects the same
        // bottom-up row order queryPixelData() returns, so flip back before uploading.
        flipRowsVertically(bytes, width, height);
        updateSubImage(texture, 0, 0, width, height, bytes);
    }
} // namespace p5
