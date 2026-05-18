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
            GLuint textureId = 0;
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);

            GLuint rboId = 0;
            glGenRenderbuffers(1, &rboId);
            glBindRenderbuffer(GL_RENDERBUFFER, rboId);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            GLuint fboId = 0;
            glGenFramebuffers(1, &fboId);
            glBindFramebuffer(GL_FRAMEBUFFER, fboId);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboId);

            const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            if (status != GL_FRAMEBUFFER_COMPLETE) {
                error("Failed to create framebuffer: " + std::to_string(status));

                glDeleteTextures(1, &textureId);
                glDeleteRenderbuffers(1, &rboId);
                glDeleteFramebuffers(1, &fboId);
                return nullptr;
            }

            return std::unique_ptr<OpenGLFramebuffer>(new OpenGLFramebuffer(fboId, rboId, textureId, {width, height}));
        }

        uint32_t getTextureId() const override
        {
            return m_texture;
        }

        uint32_t getRendererId() const override
        {
            return m_fbo;
        }

        uint2 getSize() const override
        {
            return m_size;
        }

    private:
        explicit OpenGLFramebuffer(GLuint fbo, GLuint rbo, GLuint texture, uint2 size)
            : m_fbo(fbo), m_rbo(rbo), m_texture(texture), m_size(size)
        {
        }

        GLuint m_fbo;
        GLuint m_rbo;
        GLuint m_texture;

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
