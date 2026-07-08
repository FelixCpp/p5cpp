#pragma once

#include <p5cpp/math/value2.hpp>
#include <p5cpp/graphics/texture.hpp>

#include <memory>
#include <vector>

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

        // Raw GL row order (bottom-to-top) — callers that need top-left-origin
        // image data (e.g. PNG encoding) are responsible for flipping rows.
        virtual std::vector<color_t> readPixels() const = 0;
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
        std::vector<color_t> readPixels() const;

    private:
        std::shared_ptr<FramebufferImpl> impl;
    };
} // namespace p5cpp
