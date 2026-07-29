#pragma once

#include <p5cpp/graphics/framebuffer.hpp>
#include <p5cpp/graphics/renderer.hpp>
#include <p5cpp/graphics/uniform_cache.hpp>
#include <p5cpp/graphics/shader.hpp>

#include <cstdint>

namespace p5cpp
{
    // Owns the default (single-sample, readable/writable) canvas and, once smooth() is
    // active, a parallel multisample target that shapes are actually drawn into. Bundles
    // the MSAA resource lifecycle (create/rebuild/resize) with the GL work that moves
    // pixels between the two targets, which used to be five separate GraphicsComponent
    // members/methods (m_defaultFramebuffer, m_msaaFramebuffer, m_smoothSamples,
    // rebuildMsaaFramebuffer(), isMsaaFramebuffer()) plus duplicated resolve/sync
    // sequences inline in smooth()/noSmooth()/loadPixels()/updatePixels().
    //
    // Deliberately unaware of GraphicsComponent's canvas stack: resolveToDefault()/
    // syncFromDefault() only do the GL work of moving pixels between the two targets.
    // Flushing the renderer beforehand and re-begin()ing whatever framebuffer should be
    // active afterward stays the caller's job, mirroring drawFullscreenPass() itself.
    class AntialiasedCanvas
    {
    public:
        explicit AntialiasedCanvas(uint32_t width, uint32_t height);

        // resize()/smooth()/noSmooth() return the framebuffer that was "active" (see
        // activeFramebuffer()) immediately before the call, now orphaned - or an
        // invalid (default-constructed) Framebuffer if there was nothing to hand back
        // (e.g. the very first smooth() call). The caller owns unloading it, and must
        // do so only *after* it has finished updating anything that might still
        // reference the old active framebuffer (GraphicsComponent's canvas stack) -
        // this class has no visibility into that, so it can't safely free it itself.
        // Any framebuffer that becomes orphaned but was *not* the active one (e.g.
        // m_default while smoothing is active) is freed immediately internally, since
        // nothing outside this class can be referencing it.
        [[nodiscard]] Framebuffer resize(uint32_t width, uint32_t height);
        [[nodiscard]] Framebuffer smooth(uint32_t samples);
        [[nodiscard]] Framebuffer noSmooth();

        // Frees m_default/m_msaa. For use during shutdown only, once nothing (no
        // canvas stack entry) can still be referencing either - see
        // GraphicsComponent::releaseGpuResources().
        void release();

        uint32_t samples() const { return m_samples; }
        bool isEnabled() const { return m_samples > 0; }
        bool isMsaaFramebuffer(const Framebuffer& framebuffer) const;

        // The framebuffer beginFrame()/resizeDefaultCanvas() should treat as "the" canvas:
        // the msaa target while enabled, the plain default framebuffer otherwise.
        const Framebuffer& activeFramebuffer() const { return m_samples > 0 ? m_msaa : m_default; }

        Framebuffer& defaultFramebuffer() { return m_default; }
        const Framebuffer& defaultFramebuffer() const { return m_default; }

        // Resolves the multisampled target's content into defaultFramebuffer() via
        // glBlitFramebuffer. No-op if disabled.
        void resolveToDefault();

        // Inverse: paints defaultFramebuffer()'s current content into the live msaa
        // target via a textured full-screen draw (a plain GL blit can't target a
        // multisample destination). No-op if disabled. Leaves the renderer flushed and
        // ended against the msaa framebuffer.
        void syncFromDefault(NativeRenderer& renderer, UniformCache& uniformCache, const Shader& shader);

    private:
        // Returns the previous m_msaa (possibly invalid), same handoff contract as
        // resize()/smooth()/noSmooth() above.
        [[nodiscard]] Framebuffer rebuildMsaaFramebuffer();

        Framebuffer m_default;
        Framebuffer m_msaa;
        uint32_t m_samples = 0;
    };
} // namespace p5cpp
