#include <p5cpp/graphics/image.hpp>
#include <p5cpp/application/logging.hpp>

#include <stb_image.h>
#include <stb_image_write.h>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace p5cpp
{
    std::unique_ptr<TextureImpl> loadImage(const std::filesystem::path& imageFilePath)
    {
        int width = 0;
        int height = 0;
        int channels = 0;

        // Force 4 channels (STBI_rgb_alpha) so the decoded buffer is directly usable as
        // raw R,G,B,A bytes — the same memory layout loadTexture()/Texture::upload()
        // already expect (see texture.cpp's glTexImage2D(..., GL_RGBA, GL_UNSIGNED_BYTE, ...)).
        stbi_uc* data = stbi_load(imageFilePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (data == nullptr) {
            error("Failed to load image: " + imageFilePath.string());
            return nullptr;
        }

        std::unique_ptr<TextureImpl> texture = loadTexture(static_cast<uint32_t>(width), static_cast<uint32_t>(height), reinterpret_cast<const color_t*>(data));
        stbi_image_free(data);

        return texture;
    }

    std::unique_ptr<TextureImpl> loadImage(std::span<const uint8_t> imageData)
    {
        int width = 0;
        int height = 0;
        int channels = 0;

        stbi_uc* data = stbi_load_from_memory(imageData.data(), static_cast<int>(imageData.size()), &width, &height, &channels, STBI_rgb_alpha);
        if (data == nullptr) {
            error("Failed to decode in-memory image data");
            return nullptr;
        }

        std::unique_ptr<TextureImpl> texture = loadTexture(static_cast<uint32_t>(width), static_cast<uint32_t>(height), reinterpret_cast<const color_t*>(data));
        stbi_image_free(data);

        return texture;
    }

    bool saveImage(const std::filesystem::path& imageFilePath, const Framebuffer& framebuffer)
    {
        const uint2 size = framebuffer.getSize();
        std::vector<color_t> pixels = framebuffer.readPixels();

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
