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

    namespace detail
    {
        // Two genuinely different GL resource shapes hide behind this: a regular
        // framebuffer (FBO + sampleable color Texture + depth/stencil renderbuffer) and
        // a multisample one (FBO + two multisample renderbuffers, not sampleable/readable
        // directly - see createMultisampleFramebuffer()). Defined only in
        // framebuffer.cpp; never exposed through Framebuffer's public shape.
        struct FramebufferBackend;
    }

    class Framebuffer;

    // Regular, sampleable/readable render target.
    Framebuffer createFramebuffer(uint32_t width, uint32_t height);

    // A pure multisample render target (renderbuffer-backed color+depth/stencil, no
    // texture attachment) used only as an intermediate draw target for smooth()/
    // noSmooth() antialiasing. Its color content can't be sampled or read directly -
    // it must be resolved (glBlitFramebuffer) into a regular single-sample Framebuffer
    // first, since neither glReadPixels nor texture sampling work on multisample
    // renderbuffers.
    Framebuffer createMultisampleFramebuffer(uint32_t width, uint32_t height, uint32_t samples);

    // A GL framebuffer object handle. Copies are cheap and alias the same underlying FBO
    // (shared_ptr-backed); the FBO (and any renderbuffers/textures it owns) is deleted
    // automatically once the last copy is destroyed. Default-constructed instances are
    // "invalid" (getFramebufferId().value == 0) - see isValid().
    class Framebuffer
    {
    public:
        Framebuffer();

        bool isValid() const;
        uint2 getSize() const;
        FramebufferId getFramebufferId() const;
        const Texture* getColorTexture() const;

        // Raw GL row order (bottom-to-top) — callers that need top-left-origin
        // image data (e.g. PNG encoding) are responsible for flipping rows.
        std::vector<color_t> readPixels() const;

        // Expects the same raw GL row order (bottom-to-top) that readPixels() returns.
        void writePixels(std::span<const color_t> data);

    private:
        explicit Framebuffer(std::shared_ptr<detail::FramebufferBackend> backend);

        friend Framebuffer createFramebuffer(uint32_t width, uint32_t height);
        friend Framebuffer createMultisampleFramebuffer(uint32_t width, uint32_t height, uint32_t samples);

        std::shared_ptr<detail::FramebufferBackend> m_backend;
    };
} // namespace p5cpp
