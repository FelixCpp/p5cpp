#include <p5cpp/p5cpp.hpp>

#include <glad/glad.h>

#include <algorithm>

namespace p5
{
    Framebuffer::~Framebuffer()
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

    std::unique_ptr<Framebuffer> createFramebuffer(uint32_t width, uint32_t height, uint32_t samples)
    {
        std::unique_ptr<Texture> colorTexture = loadTextureFromMemory(width, height, {});
        if (colorTexture == nullptr) {
            return nullptr;
        }

        GLuint renderbufferId = 0;
        glGenRenderbuffers(1, &renderbufferId);
        glBindRenderbuffer(GL_RENDERBUFFER, renderbufferId);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

        GLuint framebufferId = 0;
        glGenFramebuffers(1, &framebufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture->id, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbufferId);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &framebufferId);
            glDeleteRenderbuffers(1, &renderbufferId);
            error("Failed to create framebuffer");
            return nullptr;
        }

        // MSAA is layered on top as a second, multisampled FBO that gets resolved (via
        // glBlitFramebuffer) into the FBO above whenever its pending draws are flushed
        // (Renderer::flush()) — see that function for why. No depth/stencil attachment here:
        // the depth/stencil renderbuffer above is already unused by the rendering pipeline
        // (clip() uses glScissor, not stencil testing), so there's nothing to replicate.
        GLuint msaaFramebufferId = 0;
        GLuint msaaColorRenderbufferId = 0;
        if (samples >= 2) {
            GLint maxSamples = 0;
            glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
            const uint32_t clampedSamples = std::min(samples, static_cast<uint32_t>(maxSamples));
            if (clampedSamples < samples) {
                warn("createFramebuffer() clamped samples from {} to the driver maximum of {}", samples, clampedSamples);
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
                error("createFramebuffer() failed to create a multisampled framebuffer; continuing without antialiasing");
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDeleteFramebuffers(1, &msaaFbo);
                glDeleteRenderbuffers(1, &msaaColorRbo);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        auto framebuffer = std::make_unique<Framebuffer>();
        framebuffer->id = framebufferId;
        framebuffer->depthStencilRenderbufferId = renderbufferId;
        framebuffer->msaaFramebufferId = msaaFramebufferId;
        framebuffer->msaaColorRenderbufferId = msaaColorRenderbufferId;
        framebuffer->colorTexture = std::move(colorTexture);
        framebuffer->size = uint2 {.x = width, .y = height};
        return framebuffer;
    }

    void blitFramebufferToScreen(const std::shared_ptr<Framebuffer>& framebuffer, uint32_t screenWidth, uint32_t screenHeight)
    {
        glDisable(GL_SCISSOR_TEST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer->id);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, static_cast<GLint>(framebuffer->size.x), static_cast<GLint>(framebuffer->size.y), 0, 0, static_cast<GLint>(screenWidth), static_cast<GLint>(screenHeight), GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    Pixels loadPixels(const Framebuffer& framebuffer)
    {
        return loadPixels(*framebuffer.colorTexture);
    }

    void updatePixels(Framebuffer& framebuffer, const Pixels& pixels)
    {
        updatePixels(*framebuffer.colorTexture, pixels);
    }
} // namespace p5
