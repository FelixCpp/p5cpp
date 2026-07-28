#pragma once

#include <p5cpp/math/value2.hpp>
#include <p5cpp/graphics/texture.hpp>

#include <memory>
#include <span>
#include <vector>

namespace p5cpp
{
    struct FramebufferId
    {
        uint32_t value;

        inline constexpr bool operator==(const FramebufferId& other) const = default;
        inline constexpr bool operator!=(const FramebufferId& other) const = default;
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

        // Expects the same raw GL row order (bottom-to-top) that readPixels() returns.
        virtual void writePixels(std::span<const color_t> data) = 0;
    };

    std::unique_ptr<FramebufferImpl> createFramebuffer(uint32_t width, uint32_t height);

    // A pure multisample render target (renderbuffer-backed color+depth/stencil, no
    // texture attachment) used only as an intermediate draw target for smooth()/
    // noSmooth() antialiasing. Its color content can't be sampled or read directly -
    // it must be resolved (glBlitFramebuffer) into a regular single-sample Framebuffer
    // first, since neither glReadPixels nor texture sampling work on multisample
    // renderbuffers.
    std::unique_ptr<FramebufferImpl> createMultisampleFramebuffer(uint32_t width, uint32_t height, uint32_t samples);

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
        void writePixels(std::span<const color_t> data);

    private:
        std::shared_ptr<FramebufferImpl> impl;
    };
} // namespace p5cpp
