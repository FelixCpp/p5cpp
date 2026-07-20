#pragma once

#include <p5cpp/graphics/color.hpp>
#include <p5cpp/math/value2.hpp>

#include <memory>
#include <span>

namespace p5cpp
{
    struct TextureId
    {
        uint32_t value;

        constexpr bool operator==(const TextureId& other) const = default;
        constexpr bool operator!=(const TextureId& other) const = default;
    };

    struct TextureImpl
    {
        virtual ~TextureImpl() = default;
        virtual TextureId getTextureId() const = 0;
        virtual uint2 getSize() const = 0;

        // Expects the same raw GL row order (bottom-to-top) as Framebuffer::writePixels()
        // and the data loadImage() produces — row 0 of `data` becomes v=0 (bottom) of the
        // texture. Callers holding top-left-origin pixel data (e.g. row 0 == y == 0) must
        // flip it before uploading, or sampling will be vertically mirrored relative to
        // images loaded via loadImage() or textures read back from a Framebuffer.
        virtual void upload(std::span<const color_t> data) = 0;
    };

    // See TextureImpl::upload() — `data` must already be in bottom-to-top row order.
    std::unique_ptr<TextureImpl> loadTexture(uint32_t width, uint32_t height, const color_t* data);

    class Texture : public TextureImpl
    {
    public:
        Texture();
        Texture(std::unique_ptr<TextureImpl> impl);
        Texture(std::shared_ptr<TextureImpl> impl);

        TextureId getTextureId() const override;
        uint2 getSize() const override;
        void upload(std::span<const color_t> data) override;

    private:
        std::shared_ptr<TextureImpl> impl;
    };
} // namespace p5cpp
