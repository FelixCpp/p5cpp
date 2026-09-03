#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/texture_impl.hpp>

#include <glad/glad.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <algorithm>
#include <cstring>
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

        void flipRowsVertically(std::vector<uint8_t>& pixelData, uint32_t width, uint32_t height)
        {
            const size_t rowBytes = static_cast<size_t>(width) * 4;
            for (uint32_t y = 0; y < height / 2; ++y) {
                uint8_t* top = pixelData.data() + static_cast<size_t>(y) * rowBytes;
                uint8_t* bottom = pixelData.data() + static_cast<size_t>(height - 1 - y) * rowBytes;
                std::swap_ranges(top, top + rowBytes, bottom);
            }
        }

        std::vector<uint8_t> queryPixelData(const Texture& texture)
        {
            std::vector<uint8_t> pixelData(static_cast<size_t>(texture.size.x) * static_cast<size_t>(texture.size.y) * 4);
            glBindTexture(GL_TEXTURE_2D, texture.impl->id);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData.data());
            glBindTexture(GL_TEXTURE_2D, 0);
            return pixelData;
        }
    } // namespace

    TextureImpl::~TextureImpl()
    {
        glDeleteTextures(1, &id);
    }

    std::optional<Texture> loadTexture(uint32_t width, uint32_t height, std::span<const uint8_t> data, TexturePixelFormat format)
    {
        const std::optional<GLPixelFormat> glFormatOpt = toGLPixelFormat(format);
        if (not glFormatOpt.has_value()) {
            return std::nullopt;
        }
        const GLPixelFormat& glFormat = *glFormatOpt;

        const size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * glFormat.bytesPerPixel;
        if (not data.empty() and data.size() != expectedSize) {
            error("loadTexture() data size does not match width * height * bytesPerPixel");
            return std::nullopt;
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

        auto impl = std::make_shared<TextureImpl>();
        impl->id = textureId;

        return Texture {.impl = std::move(impl), .size = uint2 {.x = width, .y = height}, .pixelFormat = format};
    }

    std::optional<Texture> loadTexture(const std::filesystem::path& filepath)
    {
        typedef decltype(&stbi_image_free) stbi_deleter;

        stbi_set_flip_vertically_on_load(1);

        const std::string filepathStr = filepath.string();
        int width, height, channels;
        std::unique_ptr<stbi_uc, stbi_deleter> pixelData(stbi_load(filepathStr.c_str(), &width, &height, &channels, STBI_rgb_alpha), &stbi_image_free);

        if (pixelData == nullptr) {
            return std::nullopt;
        }

        return loadTexture(static_cast<uint32_t>(width), static_cast<uint32_t>(height), std::span<const uint8_t>(pixelData.get(), width * height * 4), TexturePixelFormat::rgba8);
    }

    void Texture::updateSubImage(uint32_t x, uint32_t y, uint32_t width, uint32_t height, std::span<const uint8_t> data)
    {
        const std::optional<GLPixelFormat> glFormatOpt = toGLPixelFormat(pixelFormat);
        if (not glFormatOpt.has_value()) {
            return;
        }

        const GLPixelFormat& glFormat = glFormatOpt.value();

        const size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * glFormat.bytesPerPixel;
        if (data.size() != expectedSize) {
            error("updateSubImage() data size does not match width * height * bytesPerPixel");
            return;
        }
        if (x + width > size.x or y + height > size.y) {
            error("updateSubImage() region is out of bounds");
            return;
        }

        glBindTexture(GL_TEXTURE_2D, impl->id);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(x), static_cast<GLint>(y), static_cast<GLsizei>(width), static_cast<GLsizei>(height), glFormat.externalFormat, GL_UNSIGNED_BYTE, data.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    bool Texture::isValid() const
    {
        return impl != nullptr;
    }

    bool Texture::saveToFileAsPNG(const std::filesystem::path& filepath) const
    {
        const std::string filepathStr = filepath.string();
        const auto [width, height] = size;
        auto pixelData = queryPixelData(*this);
        flipRowsVertically(pixelData, width, height);
        const int result = stbi_write_png(filepathStr.c_str(), static_cast<int>(width), static_cast<int>(height), STBI_rgb_alpha, pixelData.data(), static_cast<int>(width) * 4);
        return result != 0;
    }

    bool Texture::saveToFileAsJPEG(const std::filesystem::path& filepath, int quality) const
    {
        const std::string filepathStr = filepath.string();
        const auto [width, height] = size;
        auto pixelData = queryPixelData(*this);
        flipRowsVertically(pixelData, width, height);
        const int result = stbi_write_jpg(filepathStr.c_str(), static_cast<int>(width), static_cast<int>(height), STBI_rgb_alpha, pixelData.data(), quality);
        return result != 0;
    }

    bool Texture::saveToFileAsBMP(const std::filesystem::path& filepath) const
    {
        const std::string filepathStr = filepath.string();
        const auto [width, height] = size;
        auto pixelData = queryPixelData(*this);
        flipRowsVertically(pixelData, width, height);
        const int result = stbi_write_bmp(filepathStr.c_str(), static_cast<int>(width), static_cast<int>(height), STBI_rgb_alpha, pixelData.data());
        return result != 0;
    }

    color_t Pixels::get(int32_t x, int32_t y) const
    {
        if (x < 0 or y < 0 or static_cast<uint32_t>(x) >= width or static_cast<uint32_t>(y) >= height) {
            error("Pixels::get() coordinates ({}, {}) are out of bounds for {}x{} Pixels", x, y, width, height);
            return rgba(0, 0);
        }

        return data[static_cast<size_t>(y) * width + static_cast<size_t>(x)];
    }

    void Pixels::set(int32_t x, int32_t y, color_t color)
    {
        if (x < 0 or y < 0 or static_cast<uint32_t>(x) >= width or static_cast<uint32_t>(y) >= height) {
            error("Pixels::set() coordinates ({}, {}) are out of bounds for {}x{} Pixels", x, y, width, height);
            return;
        }

        data[static_cast<size_t>(y) * width + static_cast<size_t>(x)] = color;
    }

    Pixels Texture::loadPixels() const
    {
        if (pixelFormat != TexturePixelFormat::rgba8) {
            error("loadPixels() only supports TexturePixelFormat::rgba8 textures");
            return {};
        }

        const auto [width, height] = size;
        auto bytes = queryPixelData(*this);
        flipRowsVertically(bytes, width, height);

        Pixels pixels {.width = width, .height = height, .data = std::vector<color_t>(static_cast<size_t>(width) * height)};
        for (size_t i = 0; i < pixels.data.size(); ++i) {
            pixels.data[i] = rgba(bytes[i * 4 + 0], bytes[i * 4 + 1], bytes[i * 4 + 2], bytes[i * 4 + 3]);
        }

        return pixels;
    }

    void Texture::updatePixels(const Pixels& pixels)
    {
        if (pixelFormat != TexturePixelFormat::rgba8) {
            error("updatePixels() only supports TexturePixelFormat::rgba8 textures");
            return;
        }

        const auto [width, height] = size;
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
        updateSubImage(0, 0, width, height, bytes);
    }

    PixelReader::~PixelReader()
    {
        for (PixelReaderSlot& slot : ring) {
            if (slot.fence != nullptr) {
                glDeleteSync(static_cast<GLsync>(slot.fence));
            }
            if (slot.pboId != 0) {
                glDeleteBuffers(1, &slot.pboId);
            }
        }
    }

    std::unique_ptr<PixelReader> createPixelReader(uint32_t width, uint32_t height, uint32_t ringSize)
    {
        if (ringSize < 2) {
            error("createPixelReader() requires a ringSize of at least 2, got {}", ringSize);
            return nullptr;
        }

        auto reader = std::make_unique<PixelReader>();
        reader->width = width;
        reader->height = height;
        reader->ring.resize(ringSize);

        const GLsizeiptr byteSize = static_cast<GLsizeiptr>(width) * static_cast<GLsizeiptr>(height) * 4;
        for (PixelReaderSlot& slot : reader->ring) {
            glGenBuffers(1, &slot.pboId);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, slot.pboId);
            glBufferData(GL_PIXEL_PACK_BUFFER, byteSize, nullptr, GL_STREAM_READ);
        }
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        return reader;
    }

    bool requestPixelReadback(PixelReader& reader, const Texture& texture)
    {
        if (texture.pixelFormat != TexturePixelFormat::rgba8) {
            error("requestPixelReadback() only supports TexturePixelFormat::rgba8 textures");
            return false;
        }
        if (texture.size.x != reader.width or texture.size.y != reader.height) {
            error("requestPixelReadback() texture size ({}x{}) does not match reader size ({}x{})", texture.size.x, texture.size.y, reader.width, reader.height);
            return false;
        }

        PixelReaderSlot& slot = reader.ring[reader.writeIndex];
        const bool droppedUndrained = slot.pending;
        if (droppedUndrained) {
            warn("requestPixelReadback(): dropping an undrained frame -- pollPixelReadback() isn't keeping up");
            glDeleteSync(static_cast<GLsync>(slot.fence));
            slot.fence = nullptr;
        }

        glBindBuffer(GL_PIXEL_PACK_BUFFER, slot.pboId);
        glBindTexture(GL_TEXTURE_2D, texture.impl->id);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
        slot.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        slot.pending = true;
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        reader.writeIndex = (reader.writeIndex + 1) % reader.ring.size();
        return not droppedUndrained;
    }

    std::optional<Pixels> pollPixelReadback(PixelReader& reader)
    {
        PixelReaderSlot& slot = reader.ring[reader.readIndex];
        if (not slot.pending) {
            return std::nullopt;
        }

        // Zero timeout: this only ever polls the fence's current status, never waits on it.
        const GLenum waitResult = glClientWaitSync(static_cast<GLsync>(slot.fence), 0, 0);
        if (waitResult == GL_TIMEOUT_EXPIRED or waitResult == GL_WAIT_FAILED) {
            return std::nullopt;
        }

        glDeleteSync(static_cast<GLsync>(slot.fence));
        slot.fence = nullptr;
        slot.pending = false;
        reader.readIndex = (reader.readIndex + 1) % reader.ring.size();

        glBindBuffer(GL_PIXEL_PACK_BUFFER, slot.pboId);
        const GLsizeiptr byteSize = static_cast<GLsizeiptr>(reader.width) * static_cast<GLsizeiptr>(reader.height) * 4;
        const void* mapped = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, byteSize, GL_MAP_READ_BIT);
        if (mapped == nullptr) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            error("pollPixelReadback() failed to map its PBO");
            return std::nullopt;
        }

        std::vector<uint8_t> bytes(static_cast<size_t>(byteSize));
        std::memcpy(bytes.data(), mapped, bytes.size());
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        flipRowsVertically(bytes, reader.width, reader.height);

        Pixels pixels {.width = reader.width, .height = reader.height, .data = std::vector<color_t>(static_cast<size_t>(reader.width) * reader.height)};
        for (size_t i = 0; i < pixels.data.size(); ++i) {
            pixels.data[i] = rgba(bytes[i * 4 + 0], bytes[i * 4 + 1], bytes[i * 4 + 2], bytes[i * 4 + 3]);
        }

        return pixels;
    }
} // namespace p5
