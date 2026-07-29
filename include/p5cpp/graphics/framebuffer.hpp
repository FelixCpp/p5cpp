#pragma once

#include <p5cpp/math/value2.hpp>
#include <p5cpp/graphics/texture.hpp>

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace p5cpp
{
    struct FramebufferId
    {
        uint32_t value;

        inline constexpr bool operator==(const FramebufferId& other) const = default;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct Framebuffer
    {
        FramebufferId id;
        uint2 size;
        Texture colorTexture;
        bool multisample;
        uint32_t depthStencilRenderbufferId;
        uint32_t colorRenderbufferId;
    };

    Framebuffer createFramebuffer(uint32_t width, uint32_t height);
    Framebuffer createMultisampleFramebuffer(uint32_t width, uint32_t height, uint32_t samples);

    void unload(Framebuffer& framebuffer); // no-op if already unloaded/invalid
    bool isFramebufferValid(const Framebuffer& framebuffer);
    std::vector<color_t> readPixels(const Framebuffer& framebuffer);
    void writePixels(Framebuffer& framebuffer, std::span<const color_t> data);

    // Decodes a PNG/JPG/BMP/... file into a GPU texture. Returns an invalid Texture
    // (see isTextureValid()) on failure.
    Texture loadImage(const std::filesystem::path& imageFilePath);
    Texture loadImage(std::span<const uint8_t> imageData);

    // Encodes to PNG. Returns false on failure.
    bool saveImage(const std::filesystem::path& imageFilePath, const Framebuffer& framebuffer);
    bool saveImage(const std::filesystem::path& imageFilePath, uint32_t width, uint32_t height, std::span<const color_t> pixels);
} // namespace p5cpp
