#pragma once

#include <p5cpp/math/value2.hpp>
#include <p5cpp/graphics/texture.hpp>

#include <memory>

namespace p5cpp
{
    struct FramebufferId
    {
        uint32_t value;
    };

    struct FramebufferImpl
    {
        virtual ~FramebufferImpl() = default;

        virtual uint2 getSize() const = 0;
        virtual FramebufferId getFramebufferId() const = 0;
        virtual const Texture* getColorTexture() const = 0;
    };

    std::unique_ptr<FramebufferImpl> createFramebuffer(uint32_t width, uint32_t height);

    class Framebuffer
    {
    public:
        Framebuffer();
        Framebuffer(std::unique_ptr<FramebufferImpl> impl);
        Framebuffer(std::shared_ptr<FramebufferImpl> impl);

        uint2 getSize() const;
        FramebufferId getFramebufferId() const;
        const Texture* getColorTexture() const;

    private:
        std::shared_ptr<FramebufferImpl> impl;
    };
} // namespace p5cpp
