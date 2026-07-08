#pragma once

#include <p5cpp/graphics/texture.hpp>
#include <p5cpp/graphics/framebuffer.hpp>

#include <cstdint>
#include <filesystem>
#include <span>

namespace p5cpp
{
    // Decodes a PNG/JPG/BMP/... file into a GPU texture. Returns nullptr on failure.
    std::unique_ptr<TextureImpl> loadImage(const std::filesystem::path& imageFilePath);
    std::unique_ptr<TextureImpl> loadImage(std::span<const uint8_t> imageData);

    // Encodes to PNG. Returns false on failure.
    bool saveImage(const std::filesystem::path& imageFilePath, const Framebuffer& framebuffer);
    bool saveImage(const std::filesystem::path& imageFilePath, uint32_t width, uint32_t height, std::span<const color_t> pixels);
} // namespace p5cpp
