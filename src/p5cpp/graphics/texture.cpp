#include <p5cpp/graphics/texture.hpp>

#include <glad/glad.h>
#include <p5cpp/graphics/image.hpp>

namespace p5cpp
{
    class OpenGLTextureImpl : public TextureImpl
    {
    public:
        explicit OpenGLTextureImpl(uint32_t width, uint32_t height, const std::span<const color_t>& data)
            : size(width, height),
              textureId(0)
        {
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<const GLchar*>(data.data()));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        ~OpenGLTextureImpl()
        {
            glDeleteTextures(1, &textureId);
        }

        uint2 getSize() const override
        {
            return size;
        }

        TextureId getTextureId() const override
        {
            return TextureId {textureId};
        }

        void upload(std::span<const color_t> data) override
        {
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
            glBindTexture(GL_TEXTURE_2D, 0);
        }

    private:
        uint2 size;
        GLuint textureId;
    };
} // namespace p5cpp

namespace p5cpp
{
    std::unique_ptr<TextureImpl> loadTexture(uint32_t width, uint32_t height, const color_t* data)
    {
        return std::make_unique<OpenGLTextureImpl>(width, height, std::span<const color_t>(data, width * height));
    }
} // namespace p5cpp

namespace p5cpp
{
    Texture::Texture()
        : impl(nullptr)
    {
    }

    Texture::Texture(std::unique_ptr<TextureImpl> impl)
        : impl(std::move(impl))
    {
    }

    Texture::Texture(std::shared_ptr<TextureImpl> impl)
        : impl(std::move(impl))
    {
    }

    uint2 Texture::getSize() const
    {
        return impl->getSize();
    }

    TextureId Texture::getTextureId() const
    {
        return impl->getTextureId();
    }

    void Texture::upload(std::span<const color_t> data)
    {
        impl->upload(data);
    }
} // namespace p5cpp
