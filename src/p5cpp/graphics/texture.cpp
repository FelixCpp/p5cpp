#include <p5cpp/p5cpp.hpp>

#include <glad/glad.h>
#include <stb_image.h>
#include <stb_image_write.h>

namespace p5
{
    namespace
    {
        struct GLPixelFormat
        {
            GLint internalFormat;
            GLenum externalFormat;
            size_t bytesPerPixel;
        };

        GLPixelFormat toGLPixelFormat(TexturePixelFormat format)
        {
            switch (format) {
                case TexturePixelFormat::rgba8: return {GL_RGBA8, GL_RGBA, 4};
                case TexturePixelFormat::r8: return {GL_R8, GL_RED, 1};
                default: throw std::runtime_error("loadTextureFromMemory() called with unknown TexturePixelFormat");
            }
        }
    } // namespace

    class OpenGLTexture : public Texture
    {
    public:
        static std::unique_ptr<Texture> create(uint32_t width, uint32_t height, std::span<const uint8_t> data, TexturePixelFormat format)
        {
            const GLPixelFormat glFormat = toGLPixelFormat(format);
            const size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * glFormat.bytesPerPixel;
            if (not data.empty() and data.size() != expectedSize) {
                error("loadTextureFromMemory() data size does not match width * height * bytesPerPixel");
                return nullptr;
            }

            GLuint textureId;
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, glFormat.internalFormat, width, height, 0, glFormat.externalFormat, GL_UNSIGNED_BYTE, data.data());
            glBindTexture(GL_TEXTURE_2D, 0);

            return std::unique_ptr<OpenGLTexture>(new OpenGLTexture(textureId, uint2 {.x = width, .y = height}, format));
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

        TexturePixelFormat getPixelFormat() const override
        {
            return m_pixelFormat;
        }

        void updateSubImage(uint32_t x, uint32_t y, uint32_t width, uint32_t height, std::span<const uint8_t> data) override
        {
            const GLPixelFormat glFormat = toGLPixelFormat(m_pixelFormat);
            const size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * glFormat.bytesPerPixel;
            if (data.size() != expectedSize) {
                error("Texture::updateSubImage() data size does not match width * height * bytesPerPixel");
                return;
            }
            if (x + width > m_size.x or y + height > m_size.y) {
                error("Texture::updateSubImage() region is out of bounds");
                return;
            }

            glBindTexture(GL_TEXTURE_2D, m_textureId);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexSubImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(x), static_cast<GLint>(y), static_cast<GLsizei>(width), static_cast<GLsizei>(height), glFormat.externalFormat, GL_UNSIGNED_BYTE, data.data());
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        std::vector<uint8_t> queryPixelData() const override
        {
            std::vector<uint8_t> pixelData(static_cast<size_t>(m_size.x) * static_cast<size_t>(m_size.y) * 4);
            glBindTexture(GL_TEXTURE_2D, m_textureId);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData.data());
            glBindTexture(GL_TEXTURE_2D, 0);
            return pixelData;
        }

    private:
        explicit OpenGLTexture(GLuint textureId, const uint2& size, TexturePixelFormat pixelFormat)
            : m_textureId(textureId), m_size(size), m_pixelFormat(pixelFormat)
        {
        }

        GLuint m_textureId;
        uint2 m_size;
        TexturePixelFormat m_pixelFormat;
    };
} // namespace p5

namespace p5
{
    std::unique_ptr<Texture> loadTextureFromMemory(uint32_t width, uint32_t height, std::span<const uint8_t> data, TexturePixelFormat format)
    {
        return OpenGLTexture::create(width, height, data, format);
    }

    std::unique_ptr<Texture> loadTextureFromFile(const std::filesystem::path& filepath)
    {
        typedef decltype(&stbi_image_free) stbi_deleter;

        stbi_set_flip_vertically_on_load(1);

        const std::string filepathStr = filepath.string();
        int width, height, channels;
        std::unique_ptr<stbi_uc, stbi_deleter> pixelData(stbi_load(filepathStr.c_str(), &width, &height, &channels, STBI_rgb_alpha), &stbi_image_free);

        if (pixelData == nullptr) {
            return nullptr;
        }

        return loadTextureFromMemory(static_cast<uint32_t>(width), static_cast<uint32_t>(height), std::span<const uint8_t>(pixelData.get(), width * height * 4));
    }

    bool saveTextureToFileAsPNG(const Texture& texture, const std::filesystem::path& filepath)
    {
        const std::string filepathStr = filepath.string();
        const auto [width, height] = texture.getSize();
        const auto pixelData = texture.queryPixelData();
        const int result = stbi_write_png(filepathStr.c_str(), static_cast<int>(width), static_cast<int>(height), STBI_rgb_alpha, pixelData.data(), static_cast<int>(width) * 4);
        return result != 0;
    }

    bool saveTextureToFileAsJPEG(const Texture& texture, const std::filesystem::path& filepath, int quality)
    {
        const std::string filepathStr = filepath.string();
        const auto [width, height] = texture.getSize();
        const auto pixelData = texture.queryPixelData();
        const int result = stbi_write_jpg(filepathStr.c_str(), static_cast<int>(width), static_cast<int>(height), STBI_rgb_alpha, pixelData.data(), quality);
        return result != 0;
    }

    bool saveTextureToFileAsBMP(const Texture& texture, const std::filesystem::path& filepath)
    {
        const std::string filepathStr = filepath.string();
        const auto [width, height] = texture.getSize();
        const auto pixelData = texture.queryPixelData();
        const int result = stbi_write_bmp(filepathStr.c_str(), static_cast<int>(width), static_cast<int>(height), STBI_rgb_alpha, pixelData.data());
        return result != 0;
    }
} // namespace p5
