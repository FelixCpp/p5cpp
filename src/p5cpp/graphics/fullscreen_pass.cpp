#include <p5cpp/graphics/fullscreen_pass.hpp>
#include <p5cpp/graphics/blendmode.hpp>
#include <p5cpp/math/value4.hpp>

namespace p5cpp
{
    void drawFullscreenPass(NativeRenderer& renderer, UniformCache& uniformCache, const Framebuffer& source, const Framebuffer& dest, const Shader& shader)
    {
        renderer.begin(dest);

        const uint2 size = dest.getSize();
        const float w = static_cast<float>(size.x);
        const float h = static_cast<float>(size.y);
        const float4 white {1.0f, 1.0f, 1.0f, 1.0f};

        DrawBufferWriter& writer = renderer.getDrawScope();
        const uint32_t base = writer.getRelativeCursor();

        // Texture v-origin is bottom (GL row order), so the quad's top edge samples v=1.
        writer.pushVertex({0.0f, 0.0f}, {0.0f, 1.0f}, white);
        writer.pushVertex({w, 0.0f}, {1.0f, 1.0f}, white);
        writer.pushVertex({w, h}, {1.0f, 0.0f}, white);
        writer.pushVertex({0.0f, h}, {0.0f, 0.0f}, white);
        writer.pushTriangle(base + 0, base + 1, base + 2);
        writer.pushTriangle(base + 0, base + 2, base + 3);

        renderer.submit(writer, uniformCache.getUniforms(shader), shader, BlendMode::none, *source.getColorTexture());
        renderer.flush();
        renderer.end();
    }
} // namespace p5cpp
