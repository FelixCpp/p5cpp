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
    };

    struct TextureImpl
    {
        virtual ~TextureImpl() = default;
        virtual TextureId getTextureId() const = 0;
        virtual uint2 getSize() const = 0;
        virtual void upload(std::span<const color_t> data) = 0;
    };

    std::unique_ptr<TextureImpl> loadTexture(uint32_t width, uint32_t height, const color_t* data);

    class Texture : public TextureImpl
    {
    public:
        Texture(std::shared_ptr<TextureImpl> impl);

        TextureId getTextureId() const override;
        uint2 getSize() const override;
        void upload(std::span<const color_t> data) override;

    private:
        std::shared_ptr<TextureImpl> impl;
    };
} // namespace p5cpp
