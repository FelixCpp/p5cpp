#include <p5cpp/graphics/renderer.hpp>
#include <p5cpp/graphics/uniform_cache.hpp>

#include <glad/glad.h>

namespace p5cpp
{
    class CommitableDrawBufferWriter : public DrawBufferWriter
    {
    public:
        uint32_t getRelativeCursor() const override
        {
            return 0;
        }

        void pushVertex(const float2& position, const float2& texcoord, const float4& color) override
        {
        }

        void pushTriangle(uint32_t a, uint32_t b, uint32_t c) override
        {
        }

    private:
    };
} // namespace p5cpp

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
        }

        void end() override
        {
        }

        void flush() override
        {
        }

        void submit(DrawBufferWriter& scope, UniformCache& uniformCache, const Shader& shader, const BlendMode& blendMode, const Texture& texture) override
        {
        }

        DrawBufferWriter& getDrawScope() override
        {
            return m_drawBufferWriter;
        }

    private:
        NativeOpenGLRenderer()
            : m_drawBufferWriter()
        {
        }

        CommitableDrawBufferWriter m_drawBufferWriter;
    };
} // namespace p5cpp

namespace p5cpp
{
    std::unique_ptr<NativeRenderer> NativeRenderer::create(size_t vertexCount, size_t indexCount)
    {
        return NativeOpenGLRenderer::create(vertexCount, indexCount);
    }
} // namespace p5cpp
