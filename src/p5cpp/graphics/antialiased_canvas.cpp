#include <p5cpp/graphics/antialiased_canvas.hpp>
#include <p5cpp/graphics/fullscreen_pass.hpp>

#include <glad/glad.h>

#include <algorithm>

namespace p5cpp
{
    AntialiasedCanvas::AntialiasedCanvas(uint32_t width, uint32_t height)
        : m_default(createFramebuffer(width, height))
    {
    }

    void AntialiasedCanvas::resize(uint32_t width, uint32_t height)
    {
        m_default = createFramebuffer(width, height);
        if (m_samples > 0) {
            rebuildMsaaFramebuffer();
        }
    }

    void AntialiasedCanvas::smooth(uint32_t samples)
    {
        m_samples = std::max<uint32_t>(samples, 1);
        rebuildMsaaFramebuffer();
    }

    void AntialiasedCanvas::noSmooth()
    {
        m_samples = 0;
        m_msaa = Framebuffer();
    }

    bool AntialiasedCanvas::isMsaaFramebuffer(const Framebuffer& framebuffer) const
    {
        return m_samples > 0 && framebuffer.getFramebufferId().value == m_msaa.getFramebufferId().value;
    }

    void AntialiasedCanvas::rebuildMsaaFramebuffer()
    {
        GLint maxSamples = 1;
        glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
        const uint32_t samples = std::min(m_samples, static_cast<uint32_t>(std::max(maxSamples, 1)));

        const uint2 size = m_default.getSize();
        m_msaa = Framebuffer(createMultisampleFramebuffer(size.x, size.y, samples));
    }

    void AntialiasedCanvas::resolveToDefault()
    {
        if (m_samples == 0) return;

        const uint2 size = m_default.getSize();
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msaa.getFramebufferId().value);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_default.getFramebufferId().value);
        glBlitFramebuffer(0, 0, static_cast<GLint>(size.x), static_cast<GLint>(size.y), 0, 0, static_cast<GLint>(size.x), static_cast<GLint>(size.y), GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    void AntialiasedCanvas::syncFromDefault(NativeRenderer& renderer, UniformCache& uniformCache, const Shader& shader)
    {
        if (m_samples == 0) return;

        drawFullscreenPass(renderer, uniformCache, m_default, m_msaa, shader);
    }
} // namespace p5cpp
