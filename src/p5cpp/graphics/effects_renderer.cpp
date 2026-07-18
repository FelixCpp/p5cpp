#include <p5cpp/graphics/effects_renderer.hpp>
#include <p5cpp/graphics/internal_shaders.hpp>
#include <p5cpp/graphics/blendmode.hpp>
#include <p5cpp/math/value4.hpp>

#include <glad/glad.h>

#include <algorithm>

namespace p5cpp
{
    EffectsRenderer::EffectsRenderer()
        : m_blurShader(std::shared_ptr<ShaderImpl>(createBlurShader())),
          m_grayscaleShader(std::shared_ptr<ShaderImpl>(createGrayscaleShader())),
          m_invertShader(std::shared_ptr<ShaderImpl>(createInvertShader())),
          m_thresholdShader(std::shared_ptr<ShaderImpl>(createThresholdShader()))
    {
    }

    void EffectsRenderer::runEffect(NativeRenderer& renderer, UniformCache& uniformCache, Framebuffer& target, const Shader& shader)
    {
        renderer.flush();
        renderer.end();

        const uint2 size = target.getSize();
        ensureScratch(size);

        const float texelX = (size.x > 0) ? (1.0f / static_cast<float>(size.x)) : 0.0f;
        const float texelY = (size.y > 0) ? (1.0f / static_cast<float>(size.y)) : 0.0f;
        uniformCache.setUniform(shader, "u_TexelSize", uniform(texelX, texelY));

        runPass(renderer, uniformCache, target, m_scratchFramebuffers[0], shader);
        blit(m_scratchFramebuffers[0], target);

        renderer.begin(target);
    }

    void EffectsRenderer::applyBlur(NativeRenderer& renderer, UniformCache& uniformCache, Framebuffer& target, float amount)
    {
        renderer.flush();
        renderer.end();

        const uint2 size = target.getSize();
        ensureScratch(size);

        const float radius = std::max(amount, 0.0f);
        const float texelX = (size.x > 0) ? (1.0f / static_cast<float>(size.x)) : 0.0f;
        const float texelY = (size.y > 0) ? (1.0f / static_cast<float>(size.y)) : 0.0f;

        uniformCache.setUniform(m_blurShader, "u_TexelSize", uniform(texelX, texelY));
        uniformCache.setUniform(m_blurShader, "u_Radius", uniform(radius));

        uniformCache.setUniform(m_blurShader, "u_Direction", uniform(1.0f, 0.0f));
        runPass(renderer, uniformCache, target, m_scratchFramebuffers[0], m_blurShader);

        uniformCache.setUniform(m_blurShader, "u_Direction", uniform(0.0f, 1.0f));
        runPass(renderer, uniformCache, m_scratchFramebuffers[0], m_scratchFramebuffers[1], m_blurShader);

        blit(m_scratchFramebuffers[1], target);

        renderer.begin(target);
    }

    void EffectsRenderer::applyGrayscale(NativeRenderer& renderer, UniformCache& uniformCache, Framebuffer& target, float amount)
    {
        uniformCache.setUniform(m_grayscaleShader, "u_Amount", uniform(amount));
        runEffect(renderer, uniformCache, target, m_grayscaleShader);
    }

    void EffectsRenderer::applyInvert(NativeRenderer& renderer, UniformCache& uniformCache, Framebuffer& target, float amount)
    {
        uniformCache.setUniform(m_invertShader, "u_Amount", uniform(amount));
        runEffect(renderer, uniformCache, target, m_invertShader);
    }

    void EffectsRenderer::applyThreshold(NativeRenderer& renderer, UniformCache& uniformCache, Framebuffer& target, float amount)
    {
        uniformCache.setUniform(m_thresholdShader, "u_Amount", uniform(amount));
        runEffect(renderer, uniformCache, target, m_thresholdShader);
    }

    void EffectsRenderer::ensureScratch(uint2 size)
    {
        if (m_scratchSize.x == size.x && m_scratchSize.y == size.y) return;

        m_scratchFramebuffers[0] = createFramebuffer(size.x, size.y);
        m_scratchFramebuffers[1] = createFramebuffer(size.x, size.y);
        m_scratchSize = size;
    }

    // Draws a fullscreen quad sampling `source`'s color texture into `dest`, using `shader`.
    // Bypasses transform/tint/getShader() entirely — a screen-space pass is neither
    // transformed nor tinted, matching how background() bypasses them too.
    void EffectsRenderer::runPass(NativeRenderer& renderer, UniformCache& uniformCache, const Framebuffer& source, const Framebuffer& dest, const Shader& shader)
    {
        renderer.begin(dest);

        const uint2 size = dest.getSize();
        const float w = static_cast<float>(size.x);
        const float h = static_cast<float>(size.y);
        const float4 white {1.0f, 1.0f, 1.0f, 1.0f};

        DrawBufferWriter& writer = renderer.getDrawScope();
        const uint32_t base = writer.getRelativeCursor();

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

    void EffectsRenderer::blit(const Framebuffer& source, const Framebuffer& dest)
    {
        const uint2 size = dest.getSize();

        glBindFramebuffer(GL_READ_FRAMEBUFFER, source.getFramebufferId().value);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest.getFramebufferId().value);
        glBlitFramebuffer(0, 0, static_cast<GLint>(size.x), static_cast<GLint>(size.y), 0, 0, static_cast<GLint>(size.x), static_cast<GLint>(size.y), GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
} // namespace p5cpp
