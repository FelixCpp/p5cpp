#include "framebuffer.hpp"
#include <p5cpp/p5cpp.hpp>

#include <glad/glad.h>

namespace p5cpp
{
    class OpenGLFramebuffer : public Framebuffer
    {
    public:
        static std::unique_ptr<OpenGLFramebuffer> create(uint32_t physWidth, uint32_t physHeight, uint2 logicalSize = {0, 0})
        {
            std::unique_ptr<Texture> colorTexture = createTexture(physWidth, physHeight);
            if (colorTexture == nullptr) {
                error("Failed to create color texture for framebuffer");
                return nullptr;
            }

            GLuint rboId = 0;
            glGenRenderbuffers(1, &rboId);
            glBindRenderbuffer(GL_RENDERBUFFER, rboId);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, physWidth, physHeight);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            GLuint fboId = 0;
            glGenFramebuffers(1, &fboId);
            glBindFramebuffer(GL_FRAMEBUFFER, fboId);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture->getRendererId(), 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboId);

            const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            if (status != GL_FRAMEBUFFER_COMPLETE) {
                error("Failed to create framebuffer: " + std::to_string(status));
                glDeleteRenderbuffers(1, &rboId);
                glDeleteFramebuffers(1, &fboId);
                return nullptr;
            }

            const uint2 logical = (logicalSize.x > 0 && logicalSize.y > 0) ? logicalSize : uint2 {physWidth, physHeight};
            return std::unique_ptr<OpenGLFramebuffer>(new OpenGLFramebuffer(fboId, rboId, std::move(colorTexture), {physWidth, physHeight}, logical));
        }

        ~OpenGLFramebuffer() override
        {
            glDeleteFramebuffers(1, &m_fbo);
            glDeleteRenderbuffers(1, &m_rbo);
        }

        uint32_t getTextureId() const override
        {
            return m_colorTexture->getRendererId();
        }

        uint32_t getRendererId() const override
        {
            return m_fbo;
        }

        uint2 getSize() const override
        {
            return m_logicalSize;
        }

        uint2 getViewportSize() const override
        {
            return m_physicalSize;
        }

        Texture* getColorTexture() override
        {
            return m_colorTexture.get();
        }

    private:
        explicit OpenGLFramebuffer(GLuint fbo, GLuint rbo, std::unique_ptr<Texture> colorTexture, uint2 physicalSize, uint2 logicalSize)
            : m_fbo(fbo), m_rbo(rbo), m_colorTexture(std::move(colorTexture)), m_physicalSize(physicalSize), m_logicalSize(logicalSize)
        {
        }

        GLuint m_fbo;
        GLuint m_rbo;
        std::unique_ptr<Texture> m_colorTexture;

        uint2 m_physicalSize;
        uint2 m_logicalSize;
    };
} // namespace p5cpp

namespace p5cpp
{
    std::unique_ptr<Framebuffer> createFramebuffer(int width, int height)
    {
        return OpenGLFramebuffer::create(width, height);
    }

    std::unique_ptr<Framebuffer> create_window_framebuffer(int physWidth, int physHeight, int logicalWidth, int logicalHeight)
    {
        return OpenGLFramebuffer::create(physWidth, physHeight, {static_cast<uint32_t>(logicalWidth), static_cast<uint32_t>(logicalHeight)});
    }
} // namespace p5cpp

namespace p5cpp
{
    void blit_framebuffer_to_screen(const Framebuffer& source)
    {
        const auto [w, h] = source.getViewportSize();

        glBindFramebuffer(GL_READ_FRAMEBUFFER, source.getRendererId());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
} // namespace p5cpp
