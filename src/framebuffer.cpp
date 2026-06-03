#include "framebuffer.hpp"
#include "p5.hpp"

#include <glad/glad.h>

namespace p5
{
    class OpenGLFramebuffer : public Framebuffer
    {
    public:
        static std::unique_ptr<OpenGLFramebuffer> create(uint32_t width, uint32_t height)
        {
            auto colorTexture = createTexture(width, height);
            if (!colorTexture) {
                error("Failed to create color texture for framebuffer");
                return nullptr;
            }

            GLuint rboId = 0;
            glGenRenderbuffers(1, &rboId);
            glBindRenderbuffer(GL_RENDERBUFFER, rboId);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
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

            return std::unique_ptr<OpenGLFramebuffer>(new OpenGLFramebuffer(fboId, rboId, std::move(colorTexture), {width, height}));
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
            return m_size;
        }

        uint2 getViewportSize() const override
        {
            return m_size;
        }

        Texture* getColorTexture() override
        {
            return m_colorTexture.get();
        }

    private:
        explicit OpenGLFramebuffer(GLuint fbo, GLuint rbo, std::unique_ptr<Texture> colorTexture, uint2 size)
            : m_fbo(fbo), m_rbo(rbo), m_colorTexture(std::move(colorTexture)), m_size(size)
        {
        }

        GLuint m_fbo;
        GLuint m_rbo;
        std::unique_ptr<Texture> m_colorTexture;

        uint2 m_size;
    };
} // namespace p5

namespace p5
{
    std::unique_ptr<Framebuffer> createCanvas(int width, int height)
    {
        return OpenGLFramebuffer::create(width, height);
    }
} // namespace p5

namespace p5
{
    void blitRenderbufferToScreen(const Framebuffer& source, uint32_t width, uint32_t height)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, source.getRendererId());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, width, height, 0, height, width, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
} // namespace p5
