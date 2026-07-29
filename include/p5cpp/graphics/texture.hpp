#pragma once

#include <p5cpp/graphics/color.hpp>
#include <p5cpp/math/value2.hpp>

#include <span>

namespace p5cpp
{
    enum class TextureMode {
        normalized,
        image,
    };
}

namespace p5cpp
{
    struct TextureId
    {
        uint32_t value;

        constexpr bool operator==(const TextureId& other) const = default;
    };
} // namespace p5cpp

namespace p5cpp
{
    enum class TextureFormat {
        rgba8,
        r8,
    };
}

namespace p5cpp
{
    struct Texture
    {
        TextureId id;
        uint2 size;
        TextureFormat format;
    };

    Texture loadTexture(uint32_t width, uint32_t height, const color_t* data);
    Texture makeGlyphAtlasTexture(int width, int height);

    void unload(Texture& texture); // no-op if already unloaded/invalid

    bool isTextureValid(const Texture& texture);
    void upload(Texture& texture, std::span<const color_t> data);
    void updateRegion(Texture& texture, int x, int y, int width, int height, std::span<const uint8_t> data);
} // namespace p5cpp
