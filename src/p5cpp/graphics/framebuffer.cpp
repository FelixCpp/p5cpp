#include <p5cpp/graphics/framebuffer.hpp>
#include <p5cpp/application/logging.hpp>

#include <glad/glad.h>

#include <array>

namespace p5cpp
{
    Framebuffer createFramebuffer(uint32_t width, uint32_t height)
    {
        Texture colorTexture = loadTexture(width, height, nullptr);

        GLuint renderbufferId = 0;
        glGenRenderbuffers(1, &renderbufferId);
        glBindRenderbuffer(GL_RENDERBUFFER, renderbufferId);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, static_cast<GLsizei>(width), static_cast<GLsizei>(height));

        GLuint framebufferId = 0;
        glGenFramebuffers(1, &framebufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture.id.value, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            error("Failed to create framebuffer");
        }

        return Framebuffer {FramebufferId {framebufferId}, uint2 {width, height}, colorTexture, false, renderbufferId};
    }

    Framebuffer createMultisampleFramebuffer(uint32_t width, uint32_t height, uint32_t samples)
    {
        GLuint colorRenderbufferId = 0;
        glGenRenderbuffers(1, &colorRenderbufferId);
        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbufferId);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, static_cast<GLsizei>(samples), GL_RGBA8, static_cast<GLsizei>(width), static_cast<GLsizei>(height));

        GLuint depthStencilRenderbufferId = 0;
        glGenRenderbuffers(1, &depthStencilRenderbufferId);
        glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRenderbufferId);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, static_cast<GLsizei>(samples), GL_DEPTH24_STENCIL8, static_cast<GLsizei>(width), static_cast<GLsizei>(height));

        GLuint framebufferId = 0;
        glGenFramebuffers(1, &framebufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbufferId);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilRenderbufferId);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            error("Failed to create multisample framebuffer");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        return Framebuffer {FramebufferId {framebufferId}, uint2 {width, height}, Texture(), true, depthStencilRenderbufferId, colorRenderbufferId};
    }

    void unload(Framebuffer& framebuffer)
    {
        if (!isFramebufferValid(framebuffer))
            return;

        if (!framebuffer.multisample)
            unload(framebuffer.colorTexture);

        const GLuint fboId = framebuffer.id.value;
        glDeleteFramebuffers(1, &fboId);

        const std::array<GLuint, 2> renderbufferIds {framebuffer.depthStencilRenderbufferId, framebuffer.colorRenderbufferId};
        const GLsizei renderbufferCount = framebuffer.colorRenderbufferId != 0 ? 2 : 1;
        glDeleteRenderbuffers(renderbufferCount, renderbufferIds.data());

        framebuffer = Framebuffer();
    }

    bool isFramebufferValid(const Framebuffer& framebuffer)
    {
        return framebuffer.id.value != 0;
    }

    std::vector<color_t> readPixels(const Framebuffer& framebuffer)
    {
        if (framebuffer.multisample) {
            error("readPixels() called on a multisample framebuffer - resolve it first");
            return {};
        }

        std::vector<color_t> pixels(static_cast<size_t>(framebuffer.size.x) * static_cast<size_t>(framebuffer.size.y));

        GLint previousFbo = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id.value);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, static_cast<GLsizei>(framebuffer.size.x), static_cast<GLsizei>(framebuffer.size.y), GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFbo));

        return pixels;
    }

    void writePixels(Framebuffer& framebuffer, std::span<const color_t> data)
    {
        if (framebuffer.multisample) {
            error("writePixels() called on a multisample framebuffer - resolve it first");
            return;
        }

        upload(framebuffer.colorTexture, data);
    }
} // namespace p5cpp
