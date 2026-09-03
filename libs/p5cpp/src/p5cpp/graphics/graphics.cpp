#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/graphics_impl.hpp>
#include <p5cpp/graphics/texture_impl.hpp>

#include <glad/glad.h>

#include <algorithm>

namespace p5
{
    GraphicsImpl::~GraphicsImpl()
    {
        glDeleteFramebuffers(1, &id);
        glDeleteRenderbuffers(1, &depthStencilRenderbufferId);
        if (msaaFramebufferId != 0) {
            glDeleteFramebuffers(1, &msaaFramebufferId);
        }
        if (msaaColorRenderbufferId != 0) {
            glDeleteRenderbuffers(1, &msaaColorRenderbufferId);
        }
    }

    bool Graphics::isValid() const
    {
        return impl != nullptr;
    }

    std::optional<Graphics> createGraphics(uint32_t width, uint32_t height, uint32_t samples)
    {
        std::optional<Texture> colorTexture = loadTexture(width, height, {});
        if (not colorTexture.has_value()) {
            return std::nullopt;
        }

        GLuint renderbufferId = 0;
        glGenRenderbuffers(1, &renderbufferId);
        glBindRenderbuffer(GL_RENDERBUFFER, renderbufferId);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

        GLuint framebufferId = 0;
        glGenFramebuffers(1, &framebufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture->impl->id, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbufferId);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &framebufferId);
            glDeleteRenderbuffers(1, &renderbufferId);
            error("Failed to create graphics target");
            return std::nullopt;
        }

        // MSAA is layered on top as a second, multisampled FBO that gets resolved (via
        // glBlitFramebuffer) into the FBO above whenever its pending draws are flushed
        // (Renderer::flush()) — see that function for why. No depth/stencil attachment here:
        // the depth/stencil renderbuffer above is already unused by the rendering pipeline
        // (clip() uses glScissor, not the stencil buffer); kept for FBO completeness
        GLuint msaaFramebufferId = 0;
        GLuint msaaColorRenderbufferId = 0;
        if (samples >= 2) {
            GLint maxSamples = 0;
            glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
            const uint32_t clampedSamples = std::min(samples, static_cast<uint32_t>(maxSamples));
            if (clampedSamples < samples) {
                warn("createGraphics() clamped samples from {} to the driver maximum of {}", samples, clampedSamples);
            }

            GLuint msaaColorRbo = 0;
            glGenRenderbuffers(1, &msaaColorRbo);
            glBindRenderbuffer(GL_RENDERBUFFER, msaaColorRbo);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, static_cast<GLsizei>(clampedSamples), GL_RGBA8, width, height);

            GLuint msaaFbo = 0;
            glGenFramebuffers(1, &msaaFbo);
            glBindFramebuffer(GL_FRAMEBUFFER, msaaFbo);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaColorRbo);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
                msaaFramebufferId = msaaFbo;
                msaaColorRenderbufferId = msaaColorRbo;
            } else {
                error("createGraphics() failed to create a multisampled framebuffer; continuing without antialiasing");
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDeleteFramebuffers(1, &msaaFbo);
                glDeleteRenderbuffers(1, &msaaColorRbo);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        auto impl = std::make_shared<GraphicsImpl>();
        impl->id = framebufferId;
        impl->depthStencilRenderbufferId = renderbufferId;
        impl->msaaFramebufferId = msaaFramebufferId;
        impl->msaaColorRenderbufferId = msaaColorRenderbufferId;

        return Graphics {
            .impl = std::move(impl),
            .colorTexture = std::move(colorTexture).value(),
            .size = uint2 {.x = width, .y = height},
        };
    }

    void blitGraphicsToScreen(const Graphics& graphics, uint32_t screenWidth, uint32_t screenHeight)
    {
        glDisable(GL_SCISSOR_TEST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, graphics.impl->id);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, static_cast<GLint>(graphics.size.x), static_cast<GLint>(graphics.size.y), 0, 0, static_cast<GLint>(screenWidth), static_cast<GLint>(screenHeight), GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    Pixels Graphics::loadPixels() const
    {
        return colorTexture.loadPixels();
    }

    void Graphics::updatePixels(const Pixels& pixels)
    {
        colorTexture.updatePixels(pixels);
    }
} // namespace p5
