#include <p5cpp/graphics/framebuffer.hpp>
#include <p5cpp/application/logging.hpp>

#include <glad/glad.h>

namespace p5cpp
{
    class OpenGLFramebufferImpl : public FramebufferImpl
    {
    public:
        explicit OpenGLFramebufferImpl(uint32_t width, uint32_t height)
            : framebufferId(0),
              renderbufferId(0),
              colorTexture(loadTexture(width, height, nullptr))
        {
            glGenRenderbuffers(1, &renderbufferId);
            glBindRenderbuffer(GL_RENDERBUFFER, renderbufferId);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

            glGenFramebuffers(1, &framebufferId);
            glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture.getTextureId().value, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbufferId);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                error("Failed to create framebuffer");
            }
        }

        uint2 getSize() const override
        {
            return colorTexture.getSize();
        }

        FramebufferId getFramebufferId() const override
        {
            return FramebufferId {.value = framebufferId};
        }

        const Texture* getColorTexture() const override
        {
            return &colorTexture;
        }

    private:
        GLuint framebufferId;
        GLuint renderbufferId;
        Texture colorTexture;
    };
} // namespace p5cpp

namespace p5cpp
{
    std::unique_ptr<FramebufferImpl> createFramebuffer(uint32_t width, uint32_t height)
    {
        return std::make_unique<OpenGLFramebufferImpl>(width, height);
    }
} // namespace p5cpp

namespace p5cpp
{
    Framebuffer::Framebuffer()
        : impl(nullptr)
    {
    }

    Framebuffer::Framebuffer(std::unique_ptr<FramebufferImpl> impl)
        : impl(std::move(impl))
    {
    }

    Framebuffer::Framebuffer(std::shared_ptr<FramebufferImpl> impl)
        : impl(std::move(impl))
    {
    }

    uint2 Framebuffer::getSize() const
    {
        return impl->getSize();
    }

    FramebufferId Framebuffer::getFramebufferId() const
    {
        return impl->getFramebufferId();
    }

    const Texture* Framebuffer::getColorTexture() const
    {
        return impl->getColorTexture();
    }
} // namespace p5cpp
