#include "p5.hpp"

#include <glad/glad.h>

namespace p5
{
    class OpenGLTexture : public Texture
    {
    public:
        static std::unique_ptr<OpenGLTexture> create(uint32_t width, uint32_t height, const std::span<const uint8_t> imageData)
        {
            GLuint textureId;
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);

            return std::unique_ptr<OpenGLTexture>(new OpenGLTexture(textureId, {width, height}));
        }

        ~OpenGLTexture() override
        {
            glDeleteTextures(1, &m_textureId);
        }

        void update(std::span<const uint8_t> imageData) override
        {
            glBindTexture(GL_TEXTURE_2D, m_textureId);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_size.x, m_size.y, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        void update(std::span<const color_t> pixelData) override
        {
            glBindTexture(GL_TEXTURE_2D, m_textureId);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_size.x, m_size.y, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, pixelData.data());
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        uint32_t getRendererId() const override
        {
            return m_textureId;
        }

        uint2 getSize() const override
        {
            return m_size;
        }

    private:
        explicit OpenGLTexture(GLuint textureId, uint2 size)
            : m_textureId(textureId), m_size(size)
        {
        }

        GLuint m_textureId;
        uint2 m_size;
    };
} // namespace p5

namespace p5
{
    std::unique_ptr<Texture> loadTexture(const std::filesystem::path& imageFilePath)
    {
        return nullptr;
    }

    std::unique_ptr<Texture> loadTexture(uint32_t width, uint32_t height, std::span<const uint8_t> imageData)
    {
        return OpenGLTexture::create(width, height, imageData);
    }

    std::unique_ptr<Texture> createTexture(uint32_t width, uint32_t height)
    {
        return OpenGLTexture::create(width, height, {});
    }
} // namespace p5
