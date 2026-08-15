#include <p5cpp/p5cpp.hpp>

#include <glad/glad.h>

namespace p5
{
    class OpenGLFramebuffer : public Framebuffer
    {
    public:
        static std::unique_ptr<Framebuffer> create(uint32_t width, uint32_t height)
        {
            std::unique_ptr<Texture> colorTexture = loadTextureFromMemory(width, height, {});

            GLuint renderbufferId = 0;
            glGenRenderbuffers(1, &renderbufferId);
            glBindRenderbuffer(GL_RENDERBUFFER, renderbufferId);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

            GLuint framebufferId = 0;
            glGenFramebuffers(1, &framebufferId);
            glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture->getTextureId(), 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbufferId);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDeleteFramebuffers(1, &framebufferId);
                glDeleteRenderbuffers(1, &renderbufferId);
                error("Failed to create framebuffer");
                return nullptr;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            return std::unique_ptr<OpenGLFramebuffer>(new OpenGLFramebuffer(framebufferId, renderbufferId, std::move(colorTexture), uint2 {.x = width, .y = height}));
        }

        OpenGLFramebuffer(const OpenGLFramebuffer&) = delete;
        OpenGLFramebuffer& operator=(const OpenGLFramebuffer&) = delete;

        ~OpenGLFramebuffer() override
        {
            glDeleteFramebuffers(1, &m_framebufferId);
            glDeleteRenderbuffers(1, &m_renderbufferId);
        }

        GLuint getFramebufferId() const override
        {
            return m_framebufferId;
        }

        std::shared_ptr<Texture> getColorTexture() const override
        {
            return m_colorTexture;
        }

        const uint2& getSize() const override
        {
            return m_size;
        }

    private:
        explicit OpenGLFramebuffer(GLuint framebufferId, GLuint renderbufferId, std::unique_ptr<Texture> colorTexture, const uint2& size)
            : m_framebufferId(framebufferId), m_renderbufferId(renderbufferId), m_colorTexture(std::move(colorTexture)), m_size(size)
        {
        }

        GLuint m_framebufferId;
        GLuint m_renderbufferId;
        std::shared_ptr<Texture> m_colorTexture;
        uint2 m_size;
    };
} // namespace p5

namespace p5
{
    void blitFramebufferToScreen(const std::shared_ptr<Framebuffer>& framebuffer, uint32_t screenWidth, uint32_t screenHeight)
    {
        glDisable(GL_SCISSOR_TEST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer->getFramebufferId());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, static_cast<GLint>(framebuffer->getSize().x), static_cast<GLint>(framebuffer->getSize().y), 0, 0, static_cast<GLint>(screenWidth), static_cast<GLint>(screenHeight), GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
} // namespace p5

namespace p5
{
    std::unique_ptr<Framebuffer> createFramebuffer(uint32_t width, uint32_t height)
    {
        return OpenGLFramebuffer::create(width, height);
    }
} // namespace p5
