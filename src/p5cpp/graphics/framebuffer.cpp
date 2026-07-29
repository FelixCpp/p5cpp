#include <p5cpp/graphics/framebuffer.hpp>
#include <p5cpp/application/logging.hpp>

#include <glad/glad.h>

namespace p5cpp::detail
{
    struct FramebufferBackend
    {
        virtual ~FramebufferBackend() = default;

        virtual uint2 getSize() const = 0;
        virtual FramebufferId getFramebufferId() const = 0;
        virtual const Texture* getColorTexture() const = 0;
        virtual std::vector<color_t> readPixels() const = 0;
        virtual void writePixels(std::span<const color_t> data) = 0;
    };
} // namespace p5cpp::detail

namespace p5cpp::detail
{
    struct RegularFramebufferBackend : FramebufferBackend
    {
        explicit RegularFramebufferBackend(uint32_t width, uint32_t height)
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

        ~RegularFramebufferBackend() override
        {
            glDeleteFramebuffers(1, &framebufferId);
            glDeleteRenderbuffers(1, &renderbufferId);
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

        std::vector<color_t> readPixels() const override
        {
            const uint2 size = colorTexture.getSize();
            std::vector<color_t> pixels(static_cast<size_t>(size.x) * static_cast<size_t>(size.y));

            GLint previousFbo = 0;
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);

            glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0, static_cast<GLsizei>(size.x), static_cast<GLsizei>(size.y), GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFbo));

            return pixels;
        }

        void writePixels(std::span<const color_t> data) override
        {
            colorTexture.upload(data);
        }

        GLuint framebufferId;
        GLuint renderbufferId;
        Texture colorTexture;
    };
} // namespace p5cpp::detail

namespace p5cpp::detail
{
    struct MultisampleFramebufferBackend : FramebufferBackend
    {
        explicit MultisampleFramebufferBackend(uint32_t width, uint32_t height, uint32_t samples)
            : framebufferId(0),
              colorRenderbufferId(0),
              depthStencilRenderbufferId(0),
              size {width, height}
        {
            glGenRenderbuffers(1, &colorRenderbufferId);
            glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbufferId);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, static_cast<GLsizei>(samples), GL_RGBA8, static_cast<GLsizei>(width), static_cast<GLsizei>(height));

            glGenRenderbuffers(1, &depthStencilRenderbufferId);
            glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRenderbufferId);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, static_cast<GLsizei>(samples), GL_DEPTH24_STENCIL8, static_cast<GLsizei>(width), static_cast<GLsizei>(height));

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
        }

        ~MultisampleFramebufferBackend() override
        {
            glDeleteFramebuffers(1, &framebufferId);
            glDeleteRenderbuffers(1, &colorRenderbufferId);
            glDeleteRenderbuffers(1, &depthStencilRenderbufferId);
        }

        uint2 getSize() const override
        {
            return size;
        }

        FramebufferId getFramebufferId() const override
        {
            return FramebufferId {.value = framebufferId};
        }

        // Unreachable in normal use: GraphicsComponent always resolves this into a
        // regular Framebuffer (see resolveMsaaToDefaultFramebuffer()) before anything
        // needs to sample or read its color content.
        const Texture* getColorTexture() const override
        {
            error("getColorTexture() called on a multisample framebuffer - resolve it first");
            return nullptr;
        }

        std::vector<color_t> readPixels() const override
        {
            error("readPixels() called on a multisample framebuffer - resolve it first");
            return {};
        }

        void writePixels(std::span<const color_t> /*data*/) override
        {
            error("writePixels() called on a multisample framebuffer - resolve it first");
        }

        GLuint framebufferId;
        GLuint colorRenderbufferId;
        GLuint depthStencilRenderbufferId;
        uint2 size;
    };
} // namespace p5cpp::detail

namespace p5cpp
{
    Framebuffer createFramebuffer(uint32_t width, uint32_t height)
    {
        return Framebuffer(std::make_shared<detail::RegularFramebufferBackend>(width, height));
    }

    Framebuffer createMultisampleFramebuffer(uint32_t width, uint32_t height, uint32_t samples)
    {
        return Framebuffer(std::make_shared<detail::MultisampleFramebufferBackend>(width, height, samples));
    }
} // namespace p5cpp

namespace p5cpp
{
    Framebuffer::Framebuffer()
        : m_backend(nullptr)
    {
    }

    Framebuffer::Framebuffer(std::shared_ptr<detail::FramebufferBackend> backend)
        : m_backend(std::move(backend))
    {
    }

    bool Framebuffer::isValid() const
    {
        return m_backend != nullptr;
    }

    uint2 Framebuffer::getSize() const
    {
        if (!m_backend) {
            return uint2 {0, 0};
        }

        return m_backend->getSize();
    }

    FramebufferId Framebuffer::getFramebufferId() const
    {
        if (!m_backend) {
            return FramebufferId {.value = 0};
        }

        return m_backend->getFramebufferId();
    }

    const Texture* Framebuffer::getColorTexture() const
    {
        if (!m_backend) {
            return nullptr;
        }

        return m_backend->getColorTexture();
    }

    std::vector<color_t> Framebuffer::readPixels() const
    {
        if (!m_backend) {
            return {};
        }

        return m_backend->readPixels();
    }

    void Framebuffer::writePixels(std::span<const color_t> data)
    {
        if (!m_backend) {
            return;
        }

        m_backend->writePixels(data);
    }
} // namespace p5cpp
