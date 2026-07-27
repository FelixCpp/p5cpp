#pragma once

#include <p5cpp/graphics/renderer.hpp>
#include <p5cpp/graphics/uniform_cache.hpp>
#include <p5cpp/graphics/framebuffer.hpp>
#include <p5cpp/graphics/shader.hpp>
#include <p5cpp/math/value2.hpp>

#include <array>

namespace p5cpp
{
    // Owns the built-in post-processing shaders (blur/grayscale/invert/threshold) and the
    // ping-pong scratch framebuffers a full-screen pass renders through. Each apply*()/
    // runEffect() call reads `target`'s current contents and writes the result back into
    // `target`. Takes the live NativeRenderer/UniformCache as call parameters rather than
    // storing references to them, so it has no constructor-order dependency on them.
    class EffectsRenderer
    {
    public:
        EffectsRenderer();

        void runEffect(NativeRenderer& renderer, UniformCache& uniformCache, Framebuffer& target, const Shader& shader);

        void applyBlur(NativeRenderer& renderer, UniformCache& uniformCache, Framebuffer& target, float amount);
        void applyGrayscale(NativeRenderer& renderer, UniformCache& uniformCache, Framebuffer& target, float amount);
        void applyInvert(NativeRenderer& renderer, UniformCache& uniformCache, Framebuffer& target, float amount);
        void applyThreshold(NativeRenderer& renderer, UniformCache& uniformCache, Framebuffer& target, float amount);

    private:
        void ensureScratch(uint2 size);
        static void blit(const Framebuffer& source, const Framebuffer& dest);

        Shader m_blurShader;
        Shader m_grayscaleShader;
        Shader m_invertShader;
        Shader m_thresholdShader;

        std::array<Framebuffer, 2> m_scratchFramebuffers;
        uint2 m_scratchSize {0, 0};
    };
} // namespace p5cpp
