#include <p5cpp/p5cpp.hpp>

#include <glad/glad.h>

namespace p5
{
    class OpenGLTexture : public Texture
    {
    public:
        static std::unique_ptr<Texture> create(uint32_t width, uint32_t height, std::span<const uint8_t> data)
        {
            // An empty span means "allocate uninitialized storage" (e.g. framebuffer color attachments);
            // any other size must match the RGBA8 buffer exactly or glTexImage2D would read out of bounds.
            const size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
            if (not data.empty() and data.size() != expectedSize) {
                throw std::runtime_error("loadTextureFromMemory() data size does not match width * height * 4");
            }

            GLuint textureId;
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
            glBindTexture(GL_TEXTURE_2D, 0);

            return std::unique_ptr<OpenGLTexture>(new OpenGLTexture(textureId, uint2 {.x = width, .y = height}));
        }

        OpenGLTexture(const OpenGLTexture&) = delete;
        OpenGLTexture& operator=(const OpenGLTexture&) = delete;

        ~OpenGLTexture() override
        {
            glDeleteTextures(1, &m_textureId);
        }

        uint32_t getTextureId() const override
        {
            return m_textureId;
        }

        const uint2& getSize() const override
        {
            return m_size;
        }

    private:
        explicit OpenGLTexture(GLuint textureId, const uint2& size)
            : m_textureId(textureId), m_size(size)
        {
        }

        GLuint m_textureId;
        uint2 m_size;
    };
} // namespace p5

namespace p5
{
    std::unique_ptr<Texture> loadTextureFromMemory(uint32_t width, uint32_t height, std::span<const uint8_t> data)
    {
        return OpenGLTexture::create(width, height, data);
    }
} // namespace p5
