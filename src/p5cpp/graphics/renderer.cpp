#include <p5cpp/graphics/renderer.hpp>
#include <p5cpp/graphics/uniform_cache.hpp>

#include <glad/glad.h>

namespace p5cpp
{
    class NativeOpenGLRenderer : public NativeRenderer
    {
    public:
        static std::unique_ptr<NativeOpenGLRenderer> create(size_t vertexCount, size_t indexCount)
        {
            return std::unique_ptr<NativeOpenGLRenderer>(new NativeOpenGLRenderer());
        }

        void begin(const Framebuffer& framebuffer) override
        {
            throw std::runtime_error("Not implemented");
        }

        void end() override
        {
            throw std::runtime_error("Not implemented");
        }

        void flush() override
        {
            throw std::runtime_error("Not implemented");
        }

        void submit(DrawBufferWriter& scope, UniformCache& uniformCache, const Shader& shader, const BlendMode& blendMode, const Texture& texture) override
        {
            throw std::runtime_error("Not implemented");
        }

        DrawBufferWriter& getDrawScope() override
        {
            throw std::runtime_error("Not implemented");
        }

    private:
        NativeOpenGLRenderer()
        {
        }
    };
} // namespace p5cpp

namespace p5cpp
{
    std::unique_ptr<NativeRenderer> NativeRenderer::create(size_t vertexCount, size_t indexCount)
    {
        return NativeOpenGLRenderer::create(vertexCount, indexCount);
    }
} // namespace p5cpp
